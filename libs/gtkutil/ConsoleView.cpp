#include "ConsoleView.h"

#include <gtk/gtk.h>
#include "gtkutil/nonmodal.h"
#include "gtkutil/IConv.h"
#include "i18n.h"
#include <boost/algorithm/string/replace.hpp>

namespace gtkutil
{

ConsoleView::ConsoleView() :
	_textView(gtk_text_view_new()),
	_end(NULL)
{
	// Remember the buffer of this textview
	_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(_textView));

	gtk_widget_set_size_request(_textView, 0, -1); // allow shrinking
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(_textView), GTK_WRAP_WORD);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(_textView), FALSE);

	widget_connect_escape_clear_focus_widget(_textView);

	g_signal_connect(G_OBJECT(_textView), "populate-popup", G_CALLBACK(onPopulatePopup), this);

	_scrolledFrame = gtkutil::ScrolledFrame(_textView);
	gtk_container_set_focus_chain(GTK_CONTAINER(_scrolledFrame), NULL);

	// Initialise tags
	const GdkColor yellow = { 0, 0xb0ff, 0xb0ff, 0x0000 };
	const GdkColor red = { 0, 0xffff, 0x0000, 0x0000 };
	const GdkColor black = { 0, 0x0000, 0x0000, 0x0000 };

	_errorTag = gtk_text_buffer_create_tag(_buffer, "red_foreground", "foreground-gdk", &red, 0);
	_warningTag = gtk_text_buffer_create_tag(_buffer, "yellow_foreground", "foreground-gdk", &yellow, 0);
	_standardTag = gtk_text_buffer_create_tag(_buffer, "black_foreground", "foreground-gdk", &black, 0);
}

void ConsoleView::appendText(const std::string& text, ETextMode mode)
{
	// Select a tag according to the log level
	GtkTextTag* tag;

	switch (mode) {
		case STANDARD:
			tag = _standardTag;
			break;
		case WARNING:
			tag = _warningTag;
			break;
		case ERROR:
			tag = _errorTag;
			break;
		default:
			tag = _standardTag;
	};

	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(_buffer, &iter);

	if (_end == NULL)
	{
		_end = gtk_text_buffer_create_mark(_buffer, "end", &iter, FALSE);
	}
	
	// GTK expects UTF8 characters, so convert the incoming string
	std::string converted = gtkutil::IConv::localeToUTF8(text);

	// Replace NULL characters, this is not caught by localeToUTF8
	boost::algorithm::replace_all(converted, "\0", "NULL");

	// Insert into the text buffer
	gtk_text_buffer_insert_with_tags(_buffer, &iter, 
		converted.c_str(), gint(converted.size()), tag, NULL);

	gtk_text_buffer_move_mark(_buffer, _end, &iter);
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(_textView), _end);
}

void ConsoleView::clear()
{
	gtk_text_buffer_set_text(_buffer, "", -1);
}

void ConsoleView::onClearConsole(GtkMenuItem* menuitem, ConsoleView* self)
{
	self->clear();
}

void ConsoleView::onPopulatePopup(GtkTextView* textview, GtkMenu* menu, ConsoleView* self)
{
	gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());

	GtkWidget* item = gtk_menu_item_new_with_label(_("Clear"));
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(onClearConsole), self);

	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
}

} // namespace gtkutil
