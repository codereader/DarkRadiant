#include "AboutDialog.h"

#include "i18n.h"
#include "igl.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "version.h"
#include "registry/registry.h"
#include "string/string.h"

#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/ScrolledFrame.h"

#include <gtkmm/alignment.h>
#include <gtkmm/button.h>
#include <gtkmm/window.h>
#include <gtkmm/image.h>
#include <gtkmm/stock.h>
#include <gtkmm/textview.h>
#include <gtkmm/box.h>
#include <gtkmmconfig.h>

#include "modulesystem/ModuleRegistry.h"
#include <boost/format.hpp>

namespace ui {

	namespace
	{
		const char* const RKEY_SHOW_BUILD_TIME = "user/showBuildTime";
		const char* const WINDOW_TITLE = N_("About DarkRadiant");
	}

AboutDialog::AboutDialog() :
	BlockingTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow())
{
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Create all the widgets
	populateWindow();
}

void AboutDialog::populateWindow()
{
	Gtk::VBox* dialogVBox = Gtk::manage(new Gtk::VBox(false, 6));

	Gtk::HBox* topHBox = Gtk::manage(new Gtk::HBox(false, 12));

	Gtk::Image* image = Gtk::manage(new Gtk::Image(GlobalUIManager().getLocalPixbuf("logo.png")));
	topHBox->pack_start(*image, false, false, 0);

	std::string date = __DATE__;
	std::string time = __TIME__;

	bool showBuildTime = registry::getValue<bool>(RKEY_SHOW_BUILD_TIME);
	std::string buildDate = (showBuildTime) ? date + " " + time : date;

	std::string appName(RADIANT_APPNAME_FULL());

	std::string appNameStr =
		(boost::format(_("<b><span size=\"large\">%s</span></b>")) % appName).str() + "\n";

	std::string buildDateStr =
		(boost::format(_("Build date: %s")) % buildDate).str() + "\n\n";

	std::string descStr = _("<b>The Dark Mod</b> (www.thedarkmod.com)\n\n"
		"This product contains software technology\n"
		"from id Software, Inc. ('id Technology').\n"
		"id Technology 2000 id Software,Inc.\n\n"
		"DarkRadiant is based on the GPL version\n"
		"of GtkRadiant (www.qeradiant.com)\n");

	Gtk::Label* title = Gtk::manage(new gtkutil::LeftAlignedLabel(appNameStr + buildDateStr + descStr));

	Gtk::Alignment* alignment = Gtk::manage(new Gtk::Alignment(0.0, 0.0, 1.0, 0.0));
	alignment->add(*title);

	topHBox->pack_start(*alignment, true, true, 0);

	dialogVBox->pack_start(*topHBox, false, false, 0);

	dialogVBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(
		_("<b>GTK+ Properties</b>"))), false, false, 0);

	Gtk::Label* gtkVersion = Gtk::manage(new gtkutil::LeftAlignedLabel(
		(boost::format(_("GTK+ Version: %d.%d.%d")) %
		gtk_major_version %
		gtk_minor_version %
		gtk_micro_version).str()
	));
	dialogVBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*gtkVersion, 18)), false, false, 0);

	Gtk::Label* gtkmmVersion = Gtk::manage(new gtkutil::LeftAlignedLabel(
		(boost::format(_("gtkmm Version: %d.%d.%d")) %
		GTKMM_MAJOR_VERSION %
		GTKMM_MINOR_VERSION %
		GTKMM_MICRO_VERSION).str()
	));
	dialogVBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*gtkmmVersion, 18)), false, false, 0);

	dialogVBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(
		_("<b>OpenGL Properties</b>"))), false, false, 0);

	// If anybody knows a better method to convert glubyte* to char*, please tell me...
	std::string vendorStr = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	std::string versionStr = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	std::string rendererStr = reinterpret_cast<const char*>(glGetString(GL_RENDERER));

	Gtk::Label* glVendor = Gtk::manage(new gtkutil::LeftAlignedLabel((boost::format(_("Vendor: %s")) % vendorStr).str()));
	Gtk::Label* glVersion = Gtk::manage(new gtkutil::LeftAlignedLabel((boost::format(_("Version: %s")) % versionStr).str()));
	Gtk::Label* glRenderer = Gtk::manage(new gtkutil::LeftAlignedLabel((boost::format(_("Renderer: %s")) % rendererStr).str()));

	dialogVBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*glVendor, 18)), false, false, 0);
	dialogVBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*glVersion, 18)), false, false, 0);
	dialogVBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*glRenderer, 18)), false, false, 0);

	// OpenGL extensions
	dialogVBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(
		_("<b>OpenGL Extensions</b>"))), false, false, 0);

	Gtk::TextView* textView = Gtk::manage(new Gtk::TextView);
	textView->set_editable(false);
	textView->get_buffer()->set_text(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));
	textView->set_wrap_mode(Gtk::WRAP_WORD);

	dialogVBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(
		*Gtk::manage(new gtkutil::ScrolledFrame(*textView)), 18, 1.0f)),
		true, true, 0);

	// DarkRadiant modules
	// OpenGL extensions
	dialogVBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(
		_("<b>DarkRadiant Modules</b>"))), false, false, 0);

	Gtk::TextView* moduleTextView = Gtk::manage(new Gtk::TextView);
	moduleTextView->set_editable(false);
	moduleTextView->get_buffer()->set_text(module::ModuleRegistry::Instance().getModuleList(", "));
	moduleTextView->set_wrap_mode(Gtk::WRAP_WORD);

	dialogVBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(
		*Gtk::manage(new gtkutil::ScrolledFrame(*moduleTextView)), 18, 1.0f)),
		true, true, 0
	);

	// Create the close button
	Gtk::HBox* buttonHBox = Gtk::manage(new Gtk::HBox(false, 0));
	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	okButton->signal_clicked().connect(sigc::mem_fun(*this, &AboutDialog::callbackClose));

	buttonHBox->pack_end(*okButton, false, false, 0);
	dialogVBox->pack_start(*buttonHBox, false, false, 0);

	add(*dialogVBox);
}

void AboutDialog::callbackClose()
{
	destroy();
}

void AboutDialog::showDialog(const cmd::ArgumentList& args)
{
	AboutDialog dialog;
	dialog.show(); // blocks
}

} // namespace ui
