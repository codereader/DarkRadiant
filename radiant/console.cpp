/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "console.h"

#include <time.h>

#include <gtk/gtktextbuffer.h>
#include <gtk/gtktextview.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkseparatormenuitem.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkversion.h>
#include "iregistry.h"

#include "gtkutil/messagebox.h"
#include "gtkutil/nonmodal.h"
#include "stream/stringstream.h"
#include "convert.h"

#include "modulesystem/ApplicationContextImpl.h"
#include "version.h"
#include "gtkmisc.h"
#include "mainframe.h"

// handle to the console log file
namespace
{
  FILE* g_hLogFile;
}

// called whenever we need to open/close/check the console log file
void Sys_LogFile(bool enable)
{
  if (enable && !g_hLogFile)
  {
    // settings say we should be logging and we don't have a log file .. so create it
    // open a file to log the console (if user prefs say so)
    // the file handle is g_hLogFile
    // the log file is erased
    std::string logFileName = GlobalRegistry().get(RKEY_SETTINGS_PATH) + "radiant.log";
    
    g_hLogFile = fopen(logFileName.c_str(), "w" );
    if (g_hLogFile != 0)
    {
      globalOutputStream() << "Started logging to " << logFileName.c_str() << "\n";
      time_t localtime;
      time(&localtime);
      globalOutputStream() << "Today is: " << ctime(&localtime)
        << "This is DarkRadiant " RADIANT_VERSION "\n";
        
        // Output the GTK+ version to the logfile
        std::string gtkVersion = intToStr(gtk_major_version) + "."; 
		gtkVersion += intToStr(gtk_minor_version) + "."; 
		gtkVersion += intToStr(gtk_micro_version);
        globalOutputStream() << "GTK+ Version: " << gtkVersion.c_str() << "\n";
    }
    else
      gtk_MessageBox (0, "Failed to create log file, check write permissions in Radiant directory.\n",
          "Console logging", eMB_OK, eMB_ICONERROR );
  }
  else if (!enable && g_hLogFile != 0)
  {
    // settings say we should not be logging but still we have an active logfile .. close it
    time_t localtime;
    time(&localtime);
    globalOutputStream() << "Closing log file at " << ctime(&localtime) << "\n";
    fclose( g_hLogFile );
    g_hLogFile = 0;
  }
}

//GtkWidget* g_console = 0;

//WidgetFocusPrinter g_consoleWidgetFocusPrinter("console");

/*GtkWidget* Console_constructWindow(GtkWindow* toplevel)
{
  GtkWidget* scr = gtk_scrolled_window_new (0, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scr), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scr), GTK_SHADOW_IN);
  gtk_widget_show(scr);

  {
    GtkWidget* text = gtk_text_view_new();
    gtk_widget_set_size_request(text, 0, -1); // allow shrinking
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
    gtk_container_add(GTK_CONTAINER (scr), text);
    gtk_widget_show(text);
    g_console = text;

    //globalExtendedASCIICharacterSet().print();

    widget_connect_escape_clear_focus_widget(g_console);

    //g_consoleWidgetFocusPrinter.connect(g_console);

    g_signal_connect(G_OBJECT(g_console), "populate-popup", G_CALLBACK(console_populate_popup), 0);
    g_signal_connect(G_OBJECT(g_console), "destroy", G_CALLBACK(destroy_set_null), &g_console);
  }

  gtk_container_set_focus_chain(GTK_CONTAINER(scr), NULL);

  return scr;
}*/

namespace ui {

// =================================================================================

class ConsoleWriter
{
	GtkTextBuffer* _buffer;
	GtkTextIter _iter;

	// The tags needed for the console output
	static GtkTextTag* errorTag;
	static GtkTextTag* warningTag;
	static GtkTextTag* standardTag;

public:
	ConsoleWriter() :
		_buffer(NULL)
	{}

	void write(const char* p, std::size_t length, int level) {
		if (g_hLogFile != 0) {
			fwrite(p, 1, length, g_hLogFile);

			if (*(p + length) == '\n') {
				fflush(g_hLogFile);
			}
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
				case SYS_VRB:
				case SYS_STD:
					tag = standardTag;
					break;
				case SYS_WRN:
					tag = warningTag;
					break;
				case SYS_ERR:
					tag = errorTag;
					break;
				default:
					tag = standardTag;
			};

			gtk_text_buffer_get_end_iter(_buffer, &_iter);

			static GtkTextMark* end = gtk_text_buffer_create_mark(_buffer, "end", &_iter, FALSE);
			
			gtk_text_buffer_insert_with_tags(_buffer, &_iter, p, gint(length), tag, 0);

			gtk_text_buffer_move_mark(_buffer, end, &_iter);
			gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(textView), end);
		}
	}

	void disconnectConsoleWindow() {
		_buffer = NULL;
	}

	static ConsoleWriter& Instance() {
		static ConsoleWriter _writer;
		return _writer;
	}
};

// Initialise the static members
GtkTextTag* ConsoleWriter::errorTag = NULL;
GtkTextTag* ConsoleWriter::warningTag = NULL;
GtkTextTag* ConsoleWriter::standardTag = NULL;

// ========================================================================

Console::Console() :
	_textView(NULL)
{}

GtkWidget* Console::construct(GtkWindow* toplevel) {
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

		//globalExtendedASCIICharacterSet().print();

		widget_connect_escape_clear_focus_widget(_textView);

		//g_consoleWidgetFocusPrinter.connect(g_console);

		g_signal_connect(G_OBJECT(_textView), "populate-popup", G_CALLBACK(console_populate_popup), 0);
		g_signal_connect(G_OBJECT(_textView), "destroy", G_CALLBACK(destroy_set_null), &_textView);
	}

	gtk_container_set_focus_chain(GTK_CONTAINER(scr), NULL);

	return scr;
}

GtkWidget* Console::getTextView() {
	return _textView;
}

void Console::shutdown() {
	gtk_widget_destroy(_textView);
	_textView = NULL;
	ConsoleWriter::Instance().disconnectConsoleWindow();
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

// =================================================================================

class ConsoleStreamBuf : 
	public std::streambuf
{
	// Internal character buffer
	char* _reserve;

	int _level;

public:
	/**
	 * greebo: Pass the level and the optional buffersize to the constructor.
	 *         Level can be something like SYS_ERR, SYS_STD, etc.
	 */
	ConsoleStreamBuf(int level, int bufferSize = 1) :
		_reserve(NULL),
		_level(level)
	{
		if (bufferSize) {
			_reserve = new char[bufferSize];
			setp(_reserve, _reserve + bufferSize);
		}
		else {
			setp(NULL, NULL);
		}

		// No input buffer, set this to NULL
		setg(NULL, NULL, NULL);
	}

	virtual ~ConsoleStreamBuf() {
		sync();

		if (_reserve != NULL) {
			delete[] _reserve;
		}
	}

protected:
	// These two get called by the base class streambuf
	virtual int_type overflow(int_type c) {
		writeToGtkBuffer();

		if (c != EOF) {
			if (pbase() == epptr()) {
				// Write just this single character
				int c1 = c;

				ConsoleWriter::Instance().write(reinterpret_cast<const char*>(&c1), 1, _level);
				//gtk_text_buffer_insert_with_tags(textBuffer, _iter, reinterpret_cast<const char*>(&c1), 1, _tag, 0);
			}
			else {
				sputc(c);
			}
		}

		return 0;
	}

	virtual int_type sync() {
		writeToGtkBuffer();
		return 0;
	}

private:
	void writeToGtkBuffer() {
		int_type charsToWrite = pptr() - pbase();
		
		if (pbase() != pptr()) {
			// Write the given characters to the GtkTextBuffer
			ConsoleWriter::Instance().write(_reserve, static_cast<std::size_t>(charsToWrite), _level);
			//gtk_text_buffer_insert_with_tags(textBuffer, _iter, _reserve, gint(charsToWrite), _tag, 0);

			setp(pbase(), epptr());
		}
	}
};

} // namespace ui

template<int LogLevel>
class ConsoleStream : 
	public std::ostream
{
public:
	ConsoleStream() : 
		std::ostream(new ui::ConsoleStreamBuf(LogLevel))
	{}

	virtual ~ConsoleStream() {
		ui::ConsoleStreamBuf* buf = static_cast<ui::ConsoleStreamBuf*>(rdbuf());
		if (buf != NULL) {
			delete buf;
		}
	}
};

typedef ConsoleStream<SYS_STD> ConsoleOutputStream;
typedef ConsoleStream<SYS_ERR> ConsoleErrorStream;
typedef ConsoleStream<SYS_WRN> ConsoleWarningStream;

/*std::size_t Sys_Print(int level, const char* buf, std::size_t length)
{
  bool contains_newline = std::find(buf, buf+length, '\n') != buf+length;

  if(level == SYS_ERR)
  {
    Sys_LogFile(true);
  }

  if (g_hLogFile != 0)
  {
    fwrite(buf, 1, length, g_hLogFile);
    if(contains_newline)
    {
      fflush(g_hLogFile);
    }
  }

  if (level != SYS_NOCON)
  {
    if (g_console != 0)
    {
      GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(g_console));
      
      GtkTextIter iter;
      gtk_text_buffer_get_end_iter(buffer, &iter);

      static GtkTextMark* end = gtk_text_buffer_create_mark(buffer, "end", &iter, FALSE);

      const GdkColor yellow = { 0, 0xb0ff, 0xb0ff, 0x0000 };
      const GdkColor red = { 0, 0xffff, 0x0000, 0x0000 };
      const GdkColor black = { 0, 0x0000, 0x0000, 0x0000 };

      static GtkTextTag* error_tag = gtk_text_buffer_create_tag (buffer, "red_foreground", "foreground-gdk", &red, 0);
      static GtkTextTag* warning_tag = gtk_text_buffer_create_tag (buffer, "yellow_foreground", "foreground-gdk", &yellow, 0);
      static GtkTextTag* standard_tag = gtk_text_buffer_create_tag (buffer, "black_foreground", "foreground-gdk", &black, 0);
      GtkTextTag* tag;
      switch (level)
      {
      case SYS_WRN:
        tag = warning_tag;
        break;
      case SYS_ERR:
        tag = error_tag;
        break;
      case SYS_STD:
      case SYS_VRB:
      default:
        tag = standard_tag;
        break;
      }


      {
        GtkTextBufferOutputStream textBuffer(buffer, &iter, tag);
        if(!globalCharacterSet().isUTF8())
        {
          BufferedTextOutputStream<GtkTextBufferOutputStream> buffered(textBuffer);
          buffered << ConvertLocaleToUTF8(StringRange(buf, buf + length));
        }
        else
        {
          textBuffer << StringRange(buf, buf + length);
        }
      }

      // update console widget immediatly if we're doing something time-consuming
      if(contains_newline)
      {
        gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(g_console), end);

        if(!ScreenUpdates_Enabled() && GTK_WIDGET_REALIZED(g_console))
        {
          ScreenUpdates_process();
        }
      }
    }
  }
  return length;
}*/


std::ostream& getSysPrintOutputStream() {
	static ConsoleOutputStream _stream;
	return _stream;
}

std::ostream& getSysPrintErrorStream() {
	static ConsoleErrorStream _stream;
	return _stream;
}
