#include "LogicEditor.h"

#include <gtk/gtk.h>
#include "gtkutil/LeftAlignedLabel.h"

namespace objectives {

namespace {
	// Widget enum
	enum {
		WIDGET_SUCCESS_LOGIC_ENTRY,
		WIDGET_FAILURE_LOGIC_ENTRY,
	};
}

LogicEditor::LogicEditor()
{
	// Create the text entry fields
	_widgets[WIDGET_SUCCESS_LOGIC_ENTRY] = gtk_entry_new();
	_widgets[WIDGET_FAILURE_LOGIC_ENTRY] = gtk_entry_new();

	// Create the labels for each text entry field
	GtkWidget* successLogicLabel = gtkutil::LeftAlignedLabel("Success Logic:");
	GtkWidget* failureLogicLabel = gtkutil::LeftAlignedLabel("Failure Logic:");

	// Pack the label and the widget into a GtkTable
	GtkWidget* table = gtk_table_new(2, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);

	int row = 0;
	
	// pack the success logic
	gtk_table_attach(GTK_TABLE(table), 
					 successLogicLabel,
					 0, 1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), 
							  _widgets[WIDGET_SUCCESS_LOGIC_ENTRY], 
							  1, 2, row, row+1);
	
	row++;

	// pack the failure logic
	gtk_table_attach(GTK_TABLE(table), 
					 failureLogicLabel,
					 0, 1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), 
							  _widgets[WIDGET_FAILURE_LOGIC_ENTRY], 
							  1, 2, row, row+1);

	// Take the table as primary widget for our clients
	_widget = table;
}

GtkWidget* LogicEditor::getWidget() {
	return _widget;
}

std::string LogicEditor::getSuccessLogicStr() {
	return gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_SUCCESS_LOGIC_ENTRY]));
}

std::string LogicEditor::getFailureLogicStr() {
	return gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_FAILURE_LOGIC_ENTRY]));
}

void LogicEditor::setSuccessLogicStr(const std::string& logicStr) {
	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_SUCCESS_LOGIC_ENTRY]), logicStr.c_str());
}

void LogicEditor::setFailureLogicStr(const std::string& logicStr) {
	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_FAILURE_LOGIC_ENTRY]), logicStr.c_str());
}

} // namespace objectives
