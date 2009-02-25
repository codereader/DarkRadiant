#include "FindShader.h"

#include "ieventmanager.h"
#include "iradiant.h"

#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/IconTextButton.h"
#include "string/string.h"

#include "ui/common/ShaderChooser.h"
#include "selection/algorithm/Shader.h"

#include <gtk/gtk.h>

namespace ui {

	namespace {
		const int FINDDLG_DEFAULT_SIZE_X = 550;
	    const int FINDDLG_DEFAULT_SIZE_Y = 100;
	   	
	   	const char* LABEL_FINDSHADER = "Find and Replace Shader";
	   	const char* LABEL_FIND = "Find:";
	   	const char* LABEL_REPLACE = "Replace:";
	   	const char* LABEL_SELECTED_ONLY = "Search current selection only";
	   	
		const std::string FOLDER_ICON = "folder16.png";
	   	
	    const std::string FINDDLG_WINDOW_TITLE = "Find & Replace Shader";
	    const std::string COUNT_TEXT = " shader(s) replaced.";
	}

FindAndReplaceShader::FindAndReplaceShader() :
	gtkutil::BlockingTransientWindow(FINDDLG_WINDOW_TITLE, GlobalRadiant().getMainWindow())
{
	gtk_window_set_default_size(GTK_WINDOW(getWindow()), FINDDLG_DEFAULT_SIZE_X, FINDDLG_DEFAULT_SIZE_Y);
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	// Create all the widgets
	populateWindow();
	
	// Propagate shortcuts to the main window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(getWindow()));
	
	// Show the window and its children
	show();
}

FindAndReplaceShader::~FindAndReplaceShader() {
	// Propagate shortcuts to the main window
	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(getWindow()));
}

void FindAndReplaceShader::populateWindow() {
	GtkWidget* dialogVBox = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(getWindow()), dialogVBox);
	
	GtkWidget* findHBox = gtk_hbox_new(FALSE, 0);
    GtkWidget* replaceHBox = gtk_hbox_new(FALSE, 0);
    
    // Pack these hboxes into an alignment so that they are indented
	GtkWidget* alignment = gtkutil::LeftAlignment(GTK_WIDGET(findHBox), 18, 1.0); 
	GtkWidget* alignment2 = gtkutil::LeftAlignment(GTK_WIDGET(replaceHBox), 18, 1.0);
	
	gtk_box_pack_start(GTK_BOX(dialogVBox), GTK_WIDGET(alignment), TRUE, TRUE, 0); 
	gtk_box_pack_start(GTK_BOX(dialogVBox), GTK_WIDGET(alignment2), TRUE, TRUE, 0);
	
	// Create the labels and pack them in the hbox
	GtkWidget* findLabel = gtkutil::LeftAlignedLabel(LABEL_FIND);
	GtkWidget* replaceLabel = gtkutil::LeftAlignedLabel(LABEL_REPLACE);
	gtk_widget_set_size_request(findLabel, 60, -1);
	gtk_widget_set_size_request(replaceLabel, 60, -1);
	
	gtk_box_pack_start(GTK_BOX(findHBox), findLabel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(replaceHBox), replaceLabel, FALSE, FALSE, 0);
	
	_findEntry = gtk_entry_new();
	_replaceEntry = gtk_entry_new();
	g_signal_connect(G_OBJECT(_findEntry), "changed", G_CALLBACK(onFindChanged), this);
	g_signal_connect(G_OBJECT(_findEntry), "changed", G_CALLBACK(onReplaceChanged), this);
	
	gtk_box_pack_start(GTK_BOX(findHBox), _findEntry, TRUE, TRUE, 6);
	gtk_box_pack_start(GTK_BOX(replaceHBox), _replaceEntry, TRUE, TRUE, 6);
		
	// Create the icon buttons to open the ShaderChooser and override the size request
	_findSelectButton = gtkutil::IconTextButton("", GlobalRadiant().getLocalPixbuf(FOLDER_ICON), false);
	gtk_widget_set_size_request(_findSelectButton, -1, -1); 
	g_signal_connect(G_OBJECT(_findSelectButton), "clicked", G_CALLBACK(onChooseFind), this);
	
	_replaceSelectButton = gtkutil::IconTextButton("", GlobalRadiant().getLocalPixbuf(FOLDER_ICON), false);
	gtk_widget_set_size_request(_replaceSelectButton, -1, -1); 
	g_signal_connect(G_OBJECT(_replaceSelectButton), "clicked", G_CALLBACK(onChooseReplace), this);
	
	gtk_box_pack_start(GTK_BOX(findHBox), _findSelectButton, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(replaceHBox), _replaceSelectButton, FALSE, FALSE, 0);
	
	GtkWidget* spacer = gtk_alignment_new(0,0,0,0);
	gtk_widget_set_usize(spacer, 10, 2);
	gtk_box_pack_start(GTK_BOX(dialogVBox), spacer, FALSE, FALSE, 0);
	
	// The checkbox for "search selected only"
	_selectedOnly = gtk_check_button_new_with_mnemonic(LABEL_SELECTED_ONLY);
	GtkWidget* alignment3 = gtkutil::LeftAlignment(GTK_WIDGET(_selectedOnly), 18, 1.0); 
	gtk_box_pack_start(GTK_BOX(dialogVBox), GTK_WIDGET(alignment3), FALSE, FALSE, 0); 
	
	// Finally, add the buttons
	gtk_box_pack_start(GTK_BOX(dialogVBox), createButtons(), FALSE, FALSE, 0);
}

GtkWidget* FindAndReplaceShader::createButtons() {
	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);

	GtkWidget* replaceButton = gtk_button_new_from_stock(GTK_STOCK_FIND_AND_REPLACE);
	GtkWidget* closeButton = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	
	g_signal_connect(G_OBJECT(replaceButton), "clicked", G_CALLBACK(onReplace), this);
	g_signal_connect(G_OBJECT(closeButton), "clicked", G_CALLBACK(onClose), this);

	gtk_box_pack_end(GTK_BOX(hbox), closeButton, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), replaceButton, FALSE, FALSE, 0);
	
	_counterLabel = gtkutil::LeftAlignedLabel("");
	gtk_misc_set_padding(GTK_MISC(_counterLabel), 18, 0);
	gtk_box_pack_start(GTK_BOX(hbox), _counterLabel, FALSE, FALSE, 0);
	
	return hbox;
}

void FindAndReplaceShader::performReplace() {
	const std::string find(gtk_entry_get_text(GTK_ENTRY(_findEntry)));
	const std::string replace(gtk_entry_get_text(GTK_ENTRY(_replaceEntry)));

	int replaced = selection::algorithm::findAndReplaceShader(
		find, replace,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_selectedOnly)) ? true : false // selected only
	);
	
	const std::string replacedStr = 
		std::string("<b>") + intToStr(replaced) + "</b>" + COUNT_TEXT;
	gtk_label_set_markup(GTK_LABEL(_counterLabel), replacedStr.c_str());
}

void FindAndReplaceShader::onChooseFind(GtkWidget* widget, FindAndReplaceShader* self) {
	// Construct the modal dialog, enters a main loop
	ShaderChooser chooser(NULL, GTK_WINDOW(self->getWindow()), self->_findEntry);
}

void FindAndReplaceShader::onChooseReplace(GtkWidget* widget, FindAndReplaceShader* self) {
	// Construct the modal dialog, enters a main loop
	ShaderChooser chooser(NULL, GTK_WINDOW(self->getWindow()), self->_replaceEntry);
}

void FindAndReplaceShader::onReplace(GtkWidget* widget, FindAndReplaceShader* self) {
	self->performReplace();
}

void FindAndReplaceShader::onClose(GtkWidget* widget, FindAndReplaceShader* self) {
	// Call the DialogWindow::destroy method and remove self from heap
	self->destroy();
}

void FindAndReplaceShader::onFindChanged(GtkEditable* editable, FindAndReplaceShader* self) {
	gtk_label_set_markup(GTK_LABEL(self->_counterLabel), "");
}

void FindAndReplaceShader::onReplaceChanged(GtkEditable* editable, FindAndReplaceShader* self) {
	gtk_label_set_markup(GTK_LABEL(self->_counterLabel), "");
}

void FindAndReplaceShader::showDialog(const cmd::ArgumentList& args) {
	// Just instantiate a new dialog, this enters a main loop
	FindAndReplaceShader dialog; 
}

} // namespace ui
