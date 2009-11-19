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

#include "gtkutil/IConv.h"
#include "gtkutil/dialog.h"
#include "gtkutil/FileChooser.h"

// =============================================================================
// File dialog

void button_clicked_entry_browse_file(GtkWidget* widget, GtkEntry* entry)
{
	gtkutil::FileChooser fileChooser(gtk_widget_get_toplevel(widget), "Choose File", true, false);

	fileChooser.setCurrentPath(gtk_entry_get_text(entry));

	std::string filename = fileChooser.display();

	if (GTK_IS_WINDOW(gtk_widget_get_toplevel(widget))) {
		gtk_window_present(GTK_WINDOW(gtk_widget_get_toplevel(widget)));
	}

	if (!filename.empty())
	{
		filename = gtkutil::IConv::filenameToUTF8(filename);
		gtk_entry_set_text(entry, filename.c_str());
	}
}

void button_clicked_entry_browse_directory(GtkWidget* widget, GtkEntry* entry)
{
	gtkutil::FileChooser fileChooser(gtk_widget_get_toplevel(widget), "Choose Directory", true, true);

	std::string curEntry = gtk_entry_get_text(entry);

	if (!path_is_absolute(curEntry.c_str())) 
	{
		curEntry.clear();
	}

	fileChooser.setCurrentPath(curEntry);

	std::string filename = fileChooser.display();

	if (GTK_IS_WINDOW(gtk_widget_get_toplevel(widget))) {
		gtk_window_present(GTK_WINDOW(gtk_widget_get_toplevel(widget)));
	}

	if (!filename.empty())
	{
		filename = gtkutil::IConv::filenameToUTF8(filename);
		gtk_entry_set_text(entry, filename.c_str());
	}
}
