#include "LogWriter.h"

#include <gtk/gtk.h>
#include "gtkutil/IConv.h"

#include "LogFile.h"
#include "Console.h"

namespace applog {

LogWriter::LogWriter() :
	_buffer(NULL)
{}

void LogWriter::write(const char* p, std::size_t length, int level) {
	// Write to the logfile if it is available
	if (LogFile::InstancePtr() != NULL) {
		LogFile::InstancePtr()->write(p, length);
	}
	
	// Check if the console is already initialised
	GtkWidget* textView = ui::Console::Instance().getTextView();
	if (textView == NULL) {
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
	}
}

void LogWriter::disconnectConsoleWindow() {
	_buffer = NULL;
}

LogWriter& LogWriter::Instance() {
	static LogWriter _writer;
	return _writer;
}

// Initialise the static members
GtkTextTag* LogWriter::errorTag = NULL;
GtkTextTag* LogWriter::warningTag = NULL;
GtkTextTag* LogWriter::standardTag = NULL;

} // namespace applog
