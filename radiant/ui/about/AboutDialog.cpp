#include "AboutDialog.h"

#include "i18n.h"
#include <gtk/gtk.h>
#include "igl.h"
#include "iregistry.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "version.h"
#include "string/string.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "modulesystem/ModuleRegistry.h"
#include <boost/format.hpp>

namespace ui {

	namespace {
		const std::string RKEY_SHOW_BUILD_TIME = "user/showBuildTime";
		const char* const WINDOW_TITLE = N_("About DarkRadiant");
	}

AboutDialog::AboutDialog() :
	BlockingTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow())
{
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	// Create all the widgets
	populateWindow();
}

void AboutDialog::populateWindow() {
	GtkWidget* dialogVBox = gtk_vbox_new(FALSE, 6); 
	
	GtkWidget* topHBox = gtk_hbox_new(FALSE, 12);
	
	GtkWidget* image = gtk_image_new_from_pixbuf(
		GlobalUIManager().getLocalPixbuf("logo.png")
	);
	gtk_box_pack_start(GTK_BOX(topHBox), image, FALSE, FALSE, 0);
	
	std::string date = __DATE__;
	std::string time = __TIME__;
	
	bool showBuildTime = GlobalRegistry().get(RKEY_SHOW_BUILD_TIME) == "1";
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

	GtkWidget* title = gtkutil::LeftAlignedLabel(appNameStr + buildDateStr + descStr);

	GtkWidget* alignment = gtk_alignment_new(0.0f, 0.0f, 1.0f, 0.0f);
	gtk_container_add(GTK_CONTAINER(alignment), title);
	gtk_box_pack_start(GTK_BOX(topHBox), alignment, TRUE, TRUE, 0);
	
	gtk_box_pack_start(GTK_BOX(dialogVBox), topHBox, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::LeftAlignedLabel(
		_("<b>GTK+ Properties</b>")), FALSE, FALSE, 0);
	
	GtkWidget* gtkVersion = gtkutil::LeftAlignedLabel(
		(boost::format(_("Version: %d.%d.%d")) % 
		gtk_major_version % 
		gtk_minor_version % 
		gtk_micro_version).str()
	);
	gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::LeftAlignment(gtkVersion, 18), FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::LeftAlignedLabel(
		_("<b>OpenGL Properties</b>")), FALSE, FALSE, 0);
	
	// If anybody knows a better method to convert glubyte* to char*, please tell me...
	std::string vendorStr = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	std::string versionStr = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	std::string rendererStr = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	
	GtkWidget* glVendor = gtkutil::LeftAlignedLabel((boost::format(_("Vendor: %s")) % vendorStr).str());
	GtkWidget* glVersion = gtkutil::LeftAlignedLabel((boost::format(_("Version: %s")) % versionStr).str());
	GtkWidget* glRenderer = gtkutil::LeftAlignedLabel((boost::format(_("Renderer: %s")) % rendererStr).str());
	
	gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::LeftAlignment(glVendor, 18), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::LeftAlignment(glVersion, 18), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::LeftAlignment(glRenderer, 18), FALSE, FALSE, 0);
	
	// OpenGL extensions
	gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::LeftAlignedLabel(
		_("<b>OpenGL Extensions</b>")), FALSE, FALSE, 0);
	
	GtkWidget* textView = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(textView), FALSE);
	gtk_text_buffer_set_text(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView)), 
		reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)), 
		-1
	);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textView), GTK_WRAP_WORD);
	
	gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::LeftAlignment(
		gtkutil::ScrolledFrame(textView), 18, 1.0f), 
		TRUE, TRUE, 0
	);
	
	// DarkRadiant modules
	// OpenGL extensions
	gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::LeftAlignedLabel(
		_("<b>DarkRadiant Modules</b>")), FALSE, FALSE, 0);
		
	GtkWidget* moduleTextView = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(moduleTextView), FALSE);
	gtk_text_buffer_set_text(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(moduleTextView)), 
		module::ModuleRegistry::Instance().getModuleList(", ").c_str(), 
		-1
	);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(moduleTextView), GTK_WRAP_WORD);
	
	gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::LeftAlignment(
		gtkutil::ScrolledFrame(moduleTextView), 18, 1.0f), 
		TRUE, TRUE, 0
	);
	
	// Create the close button
	GtkWidget* buttonHBox = gtk_hbox_new(FALSE, 0);
	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_box_pack_end(GTK_BOX(buttonHBox), okButton, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(callbackClose), this);
	
	gtk_box_pack_start(GTK_BOX(dialogVBox), buttonHBox, FALSE, FALSE, 0);
	
	gtk_container_add(GTK_CONTAINER(getWindow()), dialogVBox);
}

void AboutDialog::callbackClose(GtkWidget* widget, AboutDialog* self) {
	self->destroy();
}

void AboutDialog::showDialog(const cmd::ArgumentList& args) {
	AboutDialog dialog; 
	dialog.show(); // blocks
}

} // namespace ui
