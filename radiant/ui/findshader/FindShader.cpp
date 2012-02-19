#include "FindShader.h"

#include "i18n.h"
#include "ieventmanager.h"
#include "imainframe.h"
#include "iuimanager.h"

#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/IconTextButton.h"
#include "string/string.h"

#include "ui/common/ShaderChooser.h"
#include "selection/algorithm/Shader.h"

#include <gtkmm/entry.h>
#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>

namespace ui
{

	namespace
	{
		const int FINDDLG_DEFAULT_SIZE_X = 550;
	    const int FINDDLG_DEFAULT_SIZE_Y = 100;

		const char* const LABEL_FIND = N_("Find:");
		const char* const LABEL_REPLACE = N_("Replace:");
		const char* const LABEL_SELECTED_ONLY = N_("Search current selection only");

		const std::string FOLDER_ICON = "folder16.png";

	    const char* const FINDDLG_WINDOW_TITLE = N_("Find & Replace Shader");
	    const char* const COUNT_TEXT = N_("<b>%d</b> shader(s) replaced.");
	}

FindAndReplaceShader::FindAndReplaceShader() :
	gtkutil::BlockingTransientWindow(_(FINDDLG_WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow())
{
	set_default_size(FINDDLG_DEFAULT_SIZE_X, FINDDLG_DEFAULT_SIZE_Y);
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Create all the widgets
	populateWindow();

	// Propagate shortcuts to the main window
	GlobalEventManager().connectDialogWindow(this);
}

FindAndReplaceShader::~FindAndReplaceShader()
{
	// Propagate shortcuts to the main window
	GlobalEventManager().disconnectDialogWindow(this);
}

void FindAndReplaceShader::populateWindow()
{
	Gtk::VBox* dialogVBox = Gtk::manage(new Gtk::VBox(false, 6));
	add(*dialogVBox);

	Gtk::HBox* findHBox = Gtk::manage(new Gtk::HBox(false, 0));
    Gtk::HBox* replaceHBox = Gtk::manage(new Gtk::HBox(false, 0));

    // Pack these hboxes into an alignment so that they are indented
	Gtk::Alignment* alignment = Gtk::manage(new gtkutil::LeftAlignment(*findHBox, 18, 1.0f));
	Gtk::Alignment* alignment2 = Gtk::manage(new gtkutil::LeftAlignment(*replaceHBox, 18, 1.0f));

	dialogVBox->pack_start(*alignment, true, true, 0);
	dialogVBox->pack_start(*alignment2, true, true, 0);

	// Create the labels and pack them in the hbox
	Gtk::Label* findLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_FIND)));
	Gtk::Label* replaceLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_REPLACE)));

	findLabel->set_size_request(60, -1);
	replaceLabel->set_size_request(60, -1);

	findHBox->pack_start(*findLabel, false, false, 0);
	replaceHBox->pack_start(*replaceLabel, false, false, 0);

	_findEntry = Gtk::manage(new Gtk::Entry);
	_replaceEntry = Gtk::manage(new Gtk::Entry);

	_findEntry->signal_changed().connect(sigc::mem_fun(*this, &FindAndReplaceShader::onFindChanged));
	_replaceEntry->signal_changed().connect(sigc::mem_fun(*this, &FindAndReplaceShader::onReplaceChanged));

	findHBox->pack_start(*_findEntry, true, true, 6);
	replaceHBox->pack_start(*_replaceEntry, true, true, 6);

	// Create the icon buttons to open the ShaderChooser and override the size request
	_findSelectButton = Gtk::manage(
		new gtkutil::IconTextButton("", GlobalUIManager().getLocalPixbuf(FOLDER_ICON))
	);
	_findSelectButton->set_size_request(-1, -1);
	_findSelectButton->signal_clicked().connect(sigc::mem_fun(*this, &FindAndReplaceShader::onChooseFind));

	_replaceSelectButton = Gtk::manage(
		new gtkutil::IconTextButton("", GlobalUIManager().getLocalPixbuf(FOLDER_ICON))
	);
	_replaceSelectButton->set_size_request(-1, -1);
	_replaceSelectButton->signal_clicked().connect(sigc::mem_fun(*this, &FindAndReplaceShader::onChooseReplace));

	findHBox->pack_start(*_findSelectButton, false, false, 0);
	replaceHBox->pack_start(*_replaceSelectButton, false, false, 0);

	Gtk::Alignment* spacer = Gtk::manage(new Gtk::Alignment(0,0,0,0));
	spacer->set_size_request(10, 2);
	dialogVBox->pack_start(*spacer, false, false, 0);

	// The checkbox for "search selected only"
	_selectedOnly = Gtk::manage(new Gtk::CheckButton(_(LABEL_SELECTED_ONLY), true));

	Gtk::Alignment* alignment3 = Gtk::manage(new gtkutil::LeftAlignment(*_selectedOnly, 18, 1.0f));
	dialogVBox->pack_start(*alignment3, false, false, 0);

	// Finally, add the buttons
	dialogVBox->pack_start(createButtons(), false, false, 0);
}

Gtk::Widget& FindAndReplaceShader::createButtons()
{
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));

	Gtk::Button* replaceButton = Gtk::manage(new Gtk::Button(Gtk::Stock::FIND_AND_REPLACE));
	Gtk::Button* closeButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CLOSE));

	replaceButton->signal_clicked().connect(sigc::mem_fun(*this, &FindAndReplaceShader::onReplace));
	closeButton->signal_clicked().connect(sigc::mem_fun(*this, &FindAndReplaceShader::onClose));

	hbox->pack_end(*closeButton, false, false, 0);
	hbox->pack_end(*replaceButton, false, false, 0);

	_counterLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(""));
	_counterLabel->set_padding(18, 0);
	hbox->pack_start(*_counterLabel, false, false, 0);

	return *hbox;
}

void FindAndReplaceShader::performReplace()
{
	const std::string find(_findEntry->get_text());
	const std::string replace(_replaceEntry->get_text());

	int replaced = selection::algorithm::findAndReplaceShader(
		find, replace,
		_selectedOnly->get_active() // selected only
	);

	_counterLabel->set_markup((boost::format(_(COUNT_TEXT)) % replaced).str());
}

void FindAndReplaceShader::onChooseFind()
{
	// Construct the modal dialog, enters a main loop
	ShaderChooser chooser(getRefPtr(), _findEntry);
    chooser.show();
}

void FindAndReplaceShader::onChooseReplace()
{
	// Construct the modal dialog, enters a main loop
	ShaderChooser chooser(getRefPtr(), _replaceEntry);
    chooser.show();
}

void FindAndReplaceShader::onReplace()
{
	performReplace();
}

void FindAndReplaceShader::onClose()
{
	destroy();
}

void FindAndReplaceShader::onFindChanged()
{
	_counterLabel->set_text("");
}

void FindAndReplaceShader::onReplaceChanged()
{
	_counterLabel->set_text("");
}

void FindAndReplaceShader::showDialog(const cmd::ArgumentList& args)
{
	// Just instantiate a new dialog, this enters a main loop
	FindAndReplaceShader dialog;
	dialog.show();
}

} // namespace ui
