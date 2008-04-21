#include "Console.h"

#include "iuimanager.h"
#include "igroupdialog.h"

#include <gtk/gtk.h>
#include "gtkutil/nonmodal.h"
#include "gtkutil/IConv.h"

#include "LogLevels.h"
#include "LogWriter.h"
#include "StringLogDevice.h"

#include <boost/algorithm/string/replace.hpp>

namespace ui {

Console::Console() :
	_scrolled(gtk_scrolled_window_new(NULL, NULL))
{
	// Set the properties of the scrolled frame
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(_scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(_scrolled), GTK_SHADOW_IN);
	gtk_widget_show(_scrolled);

	// Create the textview, which acts as textbuffer
	_textView = gtk_text_view_new();
	gtk_widget_set_size_request(_textView, 0, -1); // allow shrinking
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(_textView), GTK_WRAP_WORD);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(_textView), FALSE);

	// Add the textview to the scrolled window
	gtk_container_add(GTK_CONTAINER(_scrolled), _textView);
	gtk_widget_show(_textView);

	widget_connect_escape_clear_focus_widget(_textView);

	g_signal_connect(G_OBJECT(_textView), "populate-popup", G_CALLBACK(console_populate_popup), this);
	g_signal_connect(G_OBJECT(_textView), "destroy", G_CALLBACK(destroy_set_null), &_textView);

	gtk_container_set_focus_chain(GTK_CONTAINER(_scrolled), NULL);

	// Remember the pointer to the textbuffer
	_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(_textView));

	// Initialise tags
	const GdkColor yellow = { 0, 0xb0ff, 0xb0ff, 0x0000 };
	const GdkColor red = { 0, 0xffff, 0x0000, 0x0000 };
	const GdkColor black = { 0, 0x0000, 0x0000, 0x0000 };

	errorTag = gtk_text_buffer_create_tag(_buffer, "red_foreground", "foreground-gdk", &red, 0);
	warningTag = gtk_text_buffer_create_tag(_buffer, "yellow_foreground", "foreground-gdk", &yellow, 0);
	standardTag = gtk_text_buffer_create_tag(_buffer, "black_foreground", "foreground-gdk", &black, 0);

	// We're ready to catch log output, register ourselves
	applog::LogWriter::Instance().attach(this);

	// Copy the temporary buffers over
	if (applog::StringLogDevice::InstancePtr() != NULL) {
		applog::StringLogDevice& logger = *applog::StringLogDevice::InstancePtr();

		for (int level = applog::SYS_VERBOSE; 
			 level < applog::SYS_NUM_LOGLEVELS; 
			 level++)
		{
			writeLog(logger.getString(static_cast<applog::ELogLevel>(level)) + "\n", 
				static_cast<applog::ELogLevel>(level));
		}
	}

	// Destruct the temporary buffer
	applog::StringLogDevice::destroy();
}

void Console::toggle() {
	GlobalGroupDialog().togglePage("console");  
}

void Console::writeLog(const std::string& outputStr, applog::ELogLevel level) {
	// Select a tag according to the log level
	GtkTextTag* tag;

	switch (level) {
		case applog::SYS_VERBOSE:
		case applog::SYS_STANDARD:
			tag = standardTag;
			break;
		case applog::SYS_WARNING:
			tag = warningTag;
			break;
		case applog::SYS_ERROR:
			tag = errorTag;
			break;
		default:
			tag = standardTag;
	};

	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(_buffer, &iter);

	static GtkTextMark* end = gtk_text_buffer_create_mark(_buffer, "end", &iter, FALSE);
	
	// GTK expects UTF8 characters, so convert the incoming string
	std::string converted = gtkutil::IConv::localeToUTF8(outputStr);

	// Replace NULL characters, this is not caught by localeToUTF8
	boost::algorithm::replace_all(converted, "\0", "NULL");

	// Insert into the text buffer
	gtk_text_buffer_insert_with_tags(_buffer, &iter, 
		converted.c_str(), gint(converted.size()), tag, NULL);

	gtk_text_buffer_move_mark(_buffer, end, &iter);
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(_textView), end);
}

GtkWidget* Console::getWidget() {
	return _scrolled;
}

void Console::shutdown() {
	applog::LogWriter::Instance().detach(this);
}

Console& Console::Instance() {
	static Console _console;
	return _console;
}

gboolean Console::destroy_set_null(GtkWindow* widget, GtkWidget** p) {
	*p = NULL;
	return FALSE;
}

void Console::onClearConsole(GtkMenuItem* menuitem, Console* self) {
	gtk_text_buffer_set_text(self->_buffer, "", -1);
}

void Console::console_populate_popup(GtkTextView* textview, GtkMenu* menu, Console* self) {
	gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());

	GtkWidget* item = gtk_menu_item_new_with_label("Clear");
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(onClearConsole), self);

	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
}

} // namespace ui
