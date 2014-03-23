#pragma once

#include <map>
#include <wx/stc/stc.h>

namespace wxutil
{

/**
 * greebo: This is a custom extension of the wxWidgets styles text control,
 * providing a few methods to make the actual code style mapping easier.
 * It's advisable to subclass this control and map the various lexer-recognised 
 * elements to a specific appearance.
 */
class SourceViewCtrl :
	public wxStyledTextCtrl
{
public:
	enum FontStyle
	{
		Normal = 1,
		Italic = 2,
		Bold = 4,
		Underline = 8,
		Hidden = 16,
	};

	// Describes a specific style (e.g. a code comment)
	struct Style
	{
		wxString foreground;
		wxString fontname;
		int fontsize;
		FontStyle fontstyle;

		Style() :
			foreground("BLACK"),
			fontname(""),
			fontsize(10),
			fontstyle(Normal)
		{};

		Style(const char* foreground_, 
				  FontStyle fontStyle_ = Normal,
				  int fontSize_ = 10,
				  const char* fontname_ = "") :
			foreground(foreground_),
			fontname(fontname_),
			fontsize(fontSize_),
			fontstyle(fontStyle_)
		{}
	};

	// Elements as recognised by the STC lexer
	enum Element
	{
		Default = 0,
		Keyword1,
		Keyword2,
		Keyword3,
		Keyword4,
		Keyword5,
		Keyword6,
		Comment,
		CommentDoc,
		CommentLine,
		SpecialComment,
		Character,
		CharacterEOL,
		String,
		StringEOL,
		Delimiter,
		Punctuation,
		Operator,
		Brace,
		Command,
		Identifier,
		Label,
		Number,
		Parameter,
		RegEx,
		UUID,
		Value,
		Preprocessor,
		Script,
		Error,
		Undefined,
		NumElements,
	};

protected:
	typedef std::map<Element, Style> StyleMap;
	StyleMap _predefinedStyles;

public:
	SourceViewCtrl(wxWindow* parent);

	virtual ~SourceViewCtrl() {}

	// Use this method to set a lexer-recognised element to a certain style
	// e.g. SetStyleMapping(0, Number), provided that the lexer is using 
	// the index 0 to represent numbers in the source.
	virtual void SetStyleMapping(int elementIndex, Element elementType);
};

/**
 * A special class providing syntax highlighting for the Python 
 * scripting language.
 */
class PythonSourceViewCtrl :
	public SourceViewCtrl
{
public:
	PythonSourceViewCtrl(wxWindow* parent);
};

/**
 * A special class providing highlighting for the Doom 3
 * material syntax.
 */
class D3MaterialSourceViewCtrl :
	public SourceViewCtrl
{
public:
	D3MaterialSourceViewCtrl(wxWindow* parent);
};

} // namespace


#include <string>
#include <vector>
#include "iregistry.h"

#include <gtkmm/scrolledwindow.h>

#ifdef HAVE_GTKSOURCEVIEW
#include <gtksourceviewmm/sourceview.h>
#include <gtksourceviewmm/sourcelanguagemanager.h>
#include <gtksourceviewmm/sourcestyleschememanager.h>
#else
#include <gtkmm/textview.h>
#endif

namespace gtkutil
{

namespace
{
	const char* const RKEY_SOURCEVIEW_STYLE = "user/ui/sourceView/style";
}

/// An editable text widget for script source code
class SourceView: public Gtk::ScrolledWindow
{
#ifdef HAVE_GTKSOURCEVIEW
	gtksourceview::SourceView* _view;
	Glib::RefPtr<gtksourceview::SourceBuffer> _buffer;
	Glib::RefPtr<gtksourceview::SourceLanguageManager> _langManager;
private:
	// Utility method to retrieve a style scheme manager, readily set up with the correct paths
	static Glib::RefPtr<gtksourceview::SourceStyleSchemeManager> getStyleSchemeManager();
	static std::string getSourceViewDataPath();
	void setStyleSchemeFromRegistry();
#else
    // With no sourceview available this is just a widget containing a text
    // buffer
    Gtk::TextView* _view;
#endif

public:

	/**
	 * Constructs a new sourceview with the given language ID as specified
	 * in the .lang files (e.g. "python").
	 *
	 * @readOnly: Set this to TRUE to disallow editing of the text buffer.
	 */
	SourceView(const std::string& language, bool readOnly);

    /// Set the text contents of the edit buffer
	void setContents(const std::string& newContents);

	/// Returns the contents of the source buffer
	std::string getContents();

	/// Clears the contents of the buffer
	void clear();

	// Utility method to retrieve a list of all available style scheme names.
	static std::list<std::string> getAvailableStyleSchemeIds();

};

} // namespace gtkutil
