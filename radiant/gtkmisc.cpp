/*
Copyright (c) 2001, Loki software, inc.
All rights reserved.
 
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:
 
Redistributions of source code must retain the above copyright notice, this list 
of conditions and the following disclaimer.
 
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.
 
Neither the name of Loki software nor the names of its contributors may be used 
to endorse or promote products derived from this software without specific prior 
written permission. 
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY 
DIRECT,INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/

//
// Small functions to help with GTK
//

#include "gtkmisc.h"

#include <gtk/gtkcolorseldialog.h>
#include <gtk/gtkentry.h>

#include "os/path.h"

#include "gtkutil/dialog.h"
#include "gtkutil/filechooser.h"

// =============================================================================
// File dialog

void button_clicked_entry_browse_file(GtkWidget* widget, GtkEntry* entry) {
	std::string filename = file_dialog(gtk_widget_get_toplevel(widget), TRUE, "Choose File", gtk_entry_get_text(entry));

	if (GTK_IS_WINDOW(gtk_widget_get_toplevel(widget))) {
		gtk_window_present(GTK_WINDOW(gtk_widget_get_toplevel(widget)));
	}

	if(!filename.empty()) {
		gtk_entry_set_text(entry, filename.c_str());
	}
}

void button_clicked_entry_browse_directory(GtkWidget* widget, GtkEntry* entry) {
	const char* text = gtk_entry_get_text(entry);
	char *dir = dir_dialog(gtk_widget_get_toplevel(widget), "Choose Directory", path_is_absolute(text) ? text : "" );

	if (GTK_IS_WINDOW(gtk_widget_get_toplevel(widget))) {
		gtk_window_present(GTK_WINDOW(gtk_widget_get_toplevel(widget)));
	}

	if(dir != 0) {
		gchar* converted = g_filename_to_utf8(dir, -1, 0, 0, 0);
		gtk_entry_set_text(entry, converted);
		g_free(dir);
		g_free(converted);
	}
}


