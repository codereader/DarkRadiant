#include "Console.h"

#include <gtk/gtk.h>
#include "gtkutil/nonmodal.h"

#include "LogWriter.h"

namespace ui {

Console::Console() :
	_textView(NULL)
{}

GtkWidget* Console::construct() {
	GtkWidget* scr = gtk_scrolled_window_new(0, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scr), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scr), GTK_SHADOW_IN);
	gtk_widget_show(scr);

	{
		_textView = gtk_text_view_new();
		gtk_widget_set_size_request(_textView, 0, -1); // allow shrinking
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(_textView), GTK_WRAP_WORD);
		gtk_text_view_set_editable(GTK_TEXT_VIEW(_textView), FALSE);
		gtk_container_add(GTK_CONTAINER(scr), _textView);
		gtk_widget_show(_textView);

		widget_connect_escape_clear_focus_widget(_textView);

		g_signal_connect(G_OBJECT(_textView), "populate-popup", G_CALLBACK(console_populate_popup), 0);
		g_signal_connect(G_OBJECT(_textView), "destroy", G_CALLBACK(destroy_set_null), &_textView);
	}

	gtk_container_set_focus_chain(GTK_CONTAINER(scr), NULL);

	return scr;
}

void Console::write(const char* buf, std::size_t length, applog::ELogLevel level) {
	// Check if the console is already initialised
	/*if (_textView == NULL) {
		// Console not yet constructed
		return;
	}

	if (_buffer == NULL) {
		// No buffer yet, try to get one
		_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));

		// Initialise tags
		const GdkColor yellow = { 0, 0xb0ff, 0xb0ff, 0x0000 };
		const GdkColor red = { 0, 0xffff, 0x0000, 0x0000 };
		const GdkColor black = { 0, 0x0000, 0x0000, 0x0000 };

		errorTag = gtk_text_buffer_create_tag(_buffer, "red_foreground", "foreground-gdk", &red, 0);
		warningTag = gtk_text_buffer_create_tag(_buffer, "yellow_foreground", "foreground-gdk", &yellow, 0);
		standardTag = gtk_text_buffer_create_tag(_buffer, "black_foreground", "foreground-gdk", &black, 0);
	}

	if (_buffer != NULL) {
		GtkTextTag* tag;

		switch (level) {
			case SYS_VERBOSE:
			case SYS_STANDARD:
				tag = standardTag;
				break;
			case SYS_WARNING:
				tag = warningTag;
				break;
			case SYS_ERROR:
				tag = errorTag;
				break;
			default:
				tag = standardTag;
		};

		GtkTextIter iter;
		gtk_text_buffer_get_end_iter(_buffer, &iter);

		static GtkTextMark* end = gtk_text_buffer_create_mark(_buffer, "end", &iter, FALSE);
		
		std::string converted = gtkutil::IConv::localeToUTF8(std::string(p, length));
		gtk_text_buffer_insert_with_tags(_buffer, &iter, converted.c_str(), gint(converted.size()), tag, 0);

		gtk_text_buffer_move_mark(_buffer, end, &iter);
		gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(textView), end);
	}*/
}

GtkWidget* Console::getTextView() {
	return _textView;
}

void Console::shutdown() {
	gtk_widget_destroy(_textView);
	_textView = NULL;
	applog::LogWriter::Instance().disconnectConsoleWindow();
}

Console& Console::Instance() {
	static Console _console;
	return _console;
}

gboolean Console::destroy_set_null(GtkWindow* widget, GtkWidget** p) {
	*p = 0;
	return FALSE;
}

void Console::console_clear() {
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Instance().getTextView()));
	gtk_text_buffer_set_text(buffer, "", -1);
}

void Console::console_populate_popup(GtkTextView* textview, GtkMenu* menu, gpointer user_data) {
	gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());

	GtkWidget* item = gtk_menu_item_new_with_label("Clear");
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(console_clear), 0);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
}

} // namespace ui
