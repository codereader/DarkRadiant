#include "AboutDialog.h"

#include <gtk/gtk.h>
#include "igl.h"
#include "iregistry.h"
#include "iradiant.h"
#include "version.h"
#include "string/string.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "modulesystem/ModuleRegistry.h"

namespace ui {

	namespace {
		const std::string RKEY_SHOW_BUILD_TIME = "user/showBuildTime";
		const std::string CMDLISTDLG_WINDOW_TITLE = "About DarkRadiant";
	}

AboutDialog::AboutDialog() :
	BlockingTransientWindow(CMDLISTDLG_WINDOW_TITLE, GlobalRadiant().getMainWindow())
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
		GlobalRadiant().getLocalPixbuf("logo.png")
	);
	gtk_box_pack_start(GTK_BOX(topHBox), image, FALSE, FALSE, 0);
	
	std::string date = __DATE__;
	std::string time = __TIME__;
	
	bool showBuildTime = GlobalRegistry().get(RKEY_SHOW_BUILD_TIME) == "1";
	std::string buildDate = (showBuildTime) ? date + " " + time : date;
	
	GtkWidget* title = gtkutil::LeftAlignedLabel(
		std::string("<b><span size=\"large\">DarkRadiant ") + RADIANT_VERSION + "</span></b>\n" +
		 "Build date: " + buildDate + "\n\n"
		"<b>The Dark Mod</b> (www.thedarkmod.com)\n\n"
		"This product contains software technology\n"
		"from id Software, Inc. ('id Technology').\n"
		"id Technology 2000 id Software,Inc.\n\n"
		"DarkRadiant is based on the GPL version\n"
		"of GtkRadiant (www.qeradiant.com)\n"
	);
	GtkWidget* alignment = gtk_alignment_new(0.0f, 0.0f, 1.0f, 0.0f);
	gtk_container_add(GTK_CONTAINER(alignment), title);
	gtk_box_pack_start(GTK_BOX(topHBox), alignment, TRUE, TRUE, 0);
	
	gtk_box_pack_start(GTK_BOX(dialogVBox), topHBox, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::LeftAlignedLabel(
		"<b>GTK+ Properties</b>"), FALSE, FALSE, 0);
	
	GtkWidget* gtkVersion = gtkutil::LeftAlignedLabel(
		"Version: " +
		intToStr(gtk_major_version) + "." + 
		intToStr(gtk_minor_version) + "." + 
		intToStr(gtk_micro_version)  
	);
	gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::LeftAlignment(gtkVersion, 18), FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::LeftAlignedLabel(
		"<b>OpenGL Properties</b>"), FALSE, FALSE, 0);
	
	// If anybody knows a better method to convert glubyte* to char*, please tell me...
	std::string vendorStr = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	std::string versionStr = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	std::string rendererStr = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	
	GtkWidget* glVendor = gtkutil::LeftAlignedLabel("Vendor: " + vendorStr);
	GtkWidget* glVersion = gtkutil::LeftAlignedLabel("Version: " + versionStr);
	GtkWidget* glRenderer = gtkutil::LeftAlignedLabel("Renderer: " + rendererStr);
	
	gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::LeftAlignment(glVendor, 18), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::LeftAlignment(glVersion, 18), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::LeftAlignment(glRenderer, 18), FALSE, FALSE, 0);
	
	// OpenGL extensions
	gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::LeftAlignedLabel(
		"<b>OpenGL Extensions</b>"), FALSE, FALSE, 0);
	
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
		"<b>DarkRadiant Modules</b>"), FALSE, FALSE, 0);
		
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

void AboutDialog::showDialog() {
	AboutDialog dialog; 
	dialog.show(); // blocks
}

} // namespace ui
