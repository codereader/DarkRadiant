#include "ScriptWindow.h"

#include "iscript.h"
#include "igroupdialog.h"
#include "iuimanager.h"
#include "iundo.h"
#include "i18n.h"

#include <wx/stc/stc.h>
#include <wx/button.h>
#include <wx/splitter.h>

#include <boost/algorithm/string/replace.hpp>

namespace script
{

namespace
{
	const char* PYTHON_KEYWORDS =
		"and assert break class continue def del elif else except exec "
		"finally for from global if import in is lambda None not or pass "
		"print raise return try while yield";

	enum FontStyle
	{
		Normal = 1,
		Italic = 2,
		Bold = 4,
	};

	// StyleInfo
	struct StyleInfo
	{
		const wxChar *name;
		const wxChar *foreground;
		const wxChar *fontname;
		int fontsize;
		FontStyle fontstyle;
	};

#define mySTC_TYPE_DEFAULT 0

#define mySTC_TYPE_WORD1 1
#define mySTC_TYPE_WORD2 2
#define mySTC_TYPE_WORD3 3
#define mySTC_TYPE_WORD4 4
#define mySTC_TYPE_WORD5 5
#define mySTC_TYPE_WORD6 6

#define mySTC_TYPE_COMMENT 7
#define mySTC_TYPE_COMMENT_DOC 8
#define mySTC_TYPE_COMMENT_LINE 9
#define mySTC_TYPE_COMMENT_SPECIAL 10

#define mySTC_TYPE_CHARACTER 11
#define mySTC_TYPE_CHARACTER_EOL 12
#define mySTC_TYPE_STRING 13
#define mySTC_TYPE_STRING_EOL 14

#define mySTC_TYPE_DELIMITER 15

#define mySTC_TYPE_PUNCTUATION 16

#define mySTC_TYPE_OPERATOR 17

#define mySTC_TYPE_BRACE 18

#define mySTC_TYPE_COMMAND 19
#define mySTC_TYPE_IDENTIFIER 20
#define mySTC_TYPE_LABEL 21
#define mySTC_TYPE_NUMBER 22
#define mySTC_TYPE_PARAMETER 23
#define mySTC_TYPE_REGEX 24
#define mySTC_TYPE_UUID 25
#define mySTC_TYPE_VALUE 26

#define mySTC_TYPE_PREPROCESSOR 27
#define mySTC_TYPE_SCRIPT 28

#define mySTC_TYPE_ERROR 29

// ------------

#define mySTC_STYLE_BOLD 1
#define mySTC_STYLE_ITALIC 2
#define mySTC_STYLE_UNDERL 4
#define mySTC_STYLE_HIDDEN 8

}

ScriptWindow::ScriptWindow(wxWindow* parent) :
	wxPanel(parent, wxID_ANY),
	_outView(new wxutil::ConsoleView(this)),
	_view(new wxStyledTextCtrl(parent, wxID_ANY))
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxSplitterWindow* vertPane = new wxSplitterWindow(this, wxID_ANY, 
		wxDefaultPosition, wxDefaultSize, wxSP_3D);
	GetSizer()->Add(vertPane, 1, wxEXPAND);

	// Edit panel has a label and a "run" button
	wxPanel* editPanel = new wxPanel(vertPane, wxID_ANY);
	editPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	wxStaticText* editLabel = new wxStaticText(editPanel, wxID_ANY, _("Python Script Input"));

	wxButton* runButton = new wxButton(editPanel, wxID_ANY, _("Run Script"));
	runButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ScriptWindow::onRunScript), NULL, this);

	editPanel->GetSizer()->Add(editLabel, 0);
	editPanel->GetSizer()->Add(_view, 1, wxEXPAND);
	editPanel->GetSizer()->Add(runButton, 0);

	// Pack the scrolled textview and the entry box to the vbox
	_outView->Reparent(vertPane);
	_view->Reparent(vertPane);

	vertPane->SplitHorizontally(editPanel, _outView);
	vertPane->SetSashPosition(150);

	// Set up styling for Python
	_view->SetLexer(wxSTC_LEX_PYTHON);

	// Predefined styles
	const StyleInfo STYLES [] =
	{
		// mySTC_TYPE_DEFAULT
		{wxT("Default"), wxT("BLACK"), wxT(""), 10, Normal},

		// mySTC_TYPE_WORD1
		{wxT("Keyword1"), wxT("BLUE"), wxT(""), 10, Bold},

		// mySTC_TYPE_WORD2
		{wxT("Keyword2"), wxT("MIDNIGHT BLUE"), wxT(""), 10, Normal},

		// mySTC_TYPE_WORD3
		{wxT("Keyword3"), wxT("CORNFLOWER BLUE"), wxT(""), 10, Normal},

		// mySTC_TYPE_WORD4
		{wxT("Keyword4"), wxT("CYAN"), wxT(""), 10, Normal},

		// mySTC_TYPE_WORD5
		{wxT("Keyword5"), wxT("DARK GREY"), wxT(""), 10, Normal},

		// mySTC_TYPE_WORD6
		{wxT("Keyword6"), wxT("GREY"), wxT(""), 10, Normal},

		// mySTC_TYPE_COMMENT
		{wxT("Comment"), wxT("FOREST GREEN"), wxT(""), 10, Normal},

		// mySTC_TYPE_COMMENT_DOC
		{wxT("Comment (Doc)"), wxT("FOREST GREEN"), wxT(""), 10, Normal},

		// mySTC_TYPE_COMMENT_LINE
		{wxT("Comment line"), wxT("FOREST GREEN"), wxT(""), 10, Normal},

		// mySTC_TYPE_COMMENT_SPECIAL
		{wxT("Special comment"), wxT("FOREST GREEN"), wxT(""), 10, Italic},

		// mySTC_TYPE_CHARACTER
		{wxT("Character"), wxT("KHAKI"), wxT(""), 10, Normal},

		// mySTC_TYPE_CHARACTER_EOL
		{wxT("Character (EOL)"), wxT("KHAKI"), wxT(""), 10, Normal},

		// mySTC_TYPE_STRING
		{wxT("String"), wxT("BROWN"), wxT(""), 10, Normal},

		// mySTC_TYPE_STRING_EOL
		{wxT("String (EOL)"), wxT("BROWN"), wxT(""), 10, Normal},

		// mySTC_TYPE_DELIMITER
		{wxT("Delimiter"), wxT("ORANGE"), wxT(""), 10, Normal},

		// mySTC_TYPE_PUNCTUATION
		{wxT("Punctuation"), wxT("ORANGE"), wxT(""), 10, Normal},

		// mySTC_TYPE_OPERATOR
		{wxT("Operator"), wxT("BLACK"), wxT(""), 10, Bold},

		// mySTC_TYPE_BRACE
		{wxT("Label"), wxT("VIOLET"), wxT(""), 10, Normal},

		// mySTC_TYPE_COMMAND
		{wxT("Command"), wxT("BLUE"), wxT(""), 10, Normal},

		// mySTC_TYPE_IDENTIFIER
		{wxT("Identifier"), wxT("BLACK"), wxT(""), 10, Normal},

		// mySTC_TYPE_LABEL
		{wxT("Label"), wxT("VIOLET"), wxT(""), 10, Normal},

		// mySTC_TYPE_NUMBER
		{wxT("Number"), wxT("SIENNA"), wxT(""), 10, Normal},

		// mySTC_TYPE_PARAMETER
		{wxT("Parameter"), wxT("VIOLET"), wxT(""), 10, Italic},

		// mySTC_TYPE_REGEX
		{wxT("Regular expression"), wxT("ORCHID"), wxT(""), 10, Normal},

		// mySTC_TYPE_UUID
		{wxT("UUID"), wxT("ORCHID"), wxT(""), 10, Normal},

		// mySTC_TYPE_VALUE
		{wxT("Value"), wxT("ORCHID"), wxT(""), 10, Italic},

		// mySTC_TYPE_PREPROCESSOR
		{wxT("Preprocessor"), wxT("GREY"), wxT(""), 10, Normal},

		// mySTC_TYPE_SCRIPT
		{wxT("Script"), wxT("DARK GREY"), wxT(""), 10, Normal},

		// mySTC_TYPE_ERROR
		{wxT("Error"), wxT("RED"), wxT(""), 10, Normal},

		// mySTC_TYPE_UNDEFINED
		{wxT("Undefined"), wxT("ORANGE"), wxT(""), 10, Normal}
	};

	_view->StyleSetForeground(0, wxColour(STYLES[mySTC_TYPE_DEFAULT].foreground));
	_view->StyleSetForeground(1, wxColour(STYLES[mySTC_TYPE_COMMENT_LINE].foreground));
	_view->StyleSetForeground(2, wxColour(STYLES[mySTC_TYPE_NUMBER].foreground));
	_view->StyleSetForeground(3, wxColour(STYLES[mySTC_TYPE_STRING].foreground));
	_view->StyleSetForeground(4, wxColour(STYLES[mySTC_TYPE_CHARACTER].foreground));
	_view->StyleSetForeground(5, wxColour(STYLES[mySTC_TYPE_WORD1].foreground));
	_view->StyleSetForeground(6, wxColour(STYLES[mySTC_TYPE_DEFAULT].foreground));
	_view->StyleSetForeground(7, wxColour(STYLES[mySTC_TYPE_DEFAULT].foreground));
	_view->StyleSetForeground(8, wxColour(STYLES[mySTC_TYPE_DEFAULT].foreground));
	_view->StyleSetForeground(9, wxColour(STYLES[mySTC_TYPE_DEFAULT].foreground));
	_view->StyleSetForeground(10, wxColour(STYLES[mySTC_TYPE_OPERATOR].foreground));
	_view->StyleSetForeground(11, wxColour(STYLES[mySTC_TYPE_IDENTIFIER].foreground));
	_view->StyleSetForeground(12, wxColour(STYLES[mySTC_TYPE_DEFAULT].foreground));
	_view->StyleSetForeground(13, wxColour(STYLES[mySTC_TYPE_STRING_EOL].foreground));

	/*_view->StyleSetBold(Nr, (curType.fontstyle & mySTC_STYLE_BOLD) > 0);
	_view->StyleSetItalic (Nr, (curType.fontstyle & mySTC_STYLE_ITALIC) > 0);
	_view->StyleSetUnderline (Nr, (curType.fontstyle & mySTC_STYLE_UNDERL) > 0);*/

	_view->SetKeyWords(0, PYTHON_KEYWORDS);

	/*for (int Nr = 0; Nr < 32; Nr++)
	{
		if (curInfo->styles[Nr].type == -1) continue;

		const StyleInfo &curType = g_StylePrefs [curInfo->styles[Nr].type];
		wxFont font (curType.fontsize, wxMODERN, wxNORMAL, wxNORMAL, false,
						curType.fontname);
		StyleSetFont (Nr, font);
		if (curType.foreground) {
			StyleSetForeground (Nr, wxColour (curType.foreground));
		}
		if (curType.background) {
			StyleSetBackground (Nr, wxColour (curType.background));
		}
		StyleSetBold (Nr, (curType.fontstyle & mySTC_STYLE_BOLD) > 0);
		StyleSetItalic (Nr, (curType.fontstyle & mySTC_STYLE_ITALIC) > 0);
		StyleSetUnderline (Nr, (curType.fontstyle & mySTC_STYLE_UNDERL) > 0);
		StyleSetVisible (Nr, (curType.fontstyle & mySTC_STYLE_HIDDEN) == 0);
		StyleSetCase (Nr, curType.lettercase);
		const char *pwords = curInfo->styles[Nr].words;
		if (pwords) {
			SetKeyWords (keywordnr, pwords);
			keywordnr += 1;
		}
	}*/
}

void ScriptWindow::toggle(const cmd::ArgumentList& args)
{
	GlobalGroupDialog().togglePage("Script");
}

void ScriptWindow::onRunScript(wxCommandEvent& ev)
{
	// Clear the output window before running
	_outView->Clear();

	// Extract the script from the input window
	std::string scriptString = _view->GetValue();

	if (scriptString.empty()) return;

	UndoableCommand cmd("runScript");

	// wxWidgets on Windows might produce \r\n, these confuse the python interpreter
	boost::algorithm::replace_all(scriptString, "\r\n", "\n");

	// Run the script
	script::ExecutionResultPtr result = GlobalScriptingSystem().executeString(scriptString);

	// Check if the output only consists of whitespace
	std::string output = boost::algorithm::replace_all_copy(result->output, "\n", "");
	boost::algorithm::replace_all(output, "\t", "");
	boost::algorithm::replace_all(output, " ", "");

	if (!result->errorOccurred && output.empty())
	{
		// If no output and no error, print at least _something_
		_outView->appendText(_("OK"), wxutil::ConsoleView::ModeStandard);
	}
	else
	{
		_outView->appendText(result->output, 
			result->errorOccurred ? wxutil::ConsoleView::ModeError : wxutil::ConsoleView::ModeStandard);
	}
}

} // namespace script
