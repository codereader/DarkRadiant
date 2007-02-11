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

#include "ieventmanager.h"

#include <gtk/gtkcolorseldialog.h>
#include <gtk/gtkentry.h>

#include "os/path.h"

#include "gtk/gtkmenuitem.h"
#include "gtk/gtkmenu.h"

#include "gtkutil/dialog.h"
#include "gtkutil/filechooser.h"
#include "gtkutil/menu.h"
#include "gtkutil/MenuItemAccelerator.h"
#include "gtkutil/SeparatorMenuItem.h"


// =============================================================================
// Misc stuff

/* greebo: Create a menu item under the given <menu> and connect it to the given <command> name
 */
GtkMenuItem* createMenuItemWithMnemonic(GtkMenu* menu, 
										const std::string& caption, 
										const std::string& commandName,
										const std::string& iconName) 
{

	GtkWidget* menuItem = NULL;

	IEventPtr event = GlobalEventManager().findEvent(commandName);

	if (!event->empty()) {
		// Retrieve an acclerator string formatted for a menu
		const std::string accelText = GlobalEventManager().getAcceleratorStr(event, true);
		 
		// Create a new menuitem
		menuItem = gtkutil::TextMenuItemAccelerator(caption, 
													accelText, 
													iconName,
													false);
	
		gtk_widget_show_all(GTK_WIDGET(menuItem));
	
		// Add the menu item to the container
		gtk_container_add(GTK_CONTAINER(menu), GTK_WIDGET(menuItem));
	
		event->connectWidget(GTK_WIDGET(menuItem));
	}
	else {
		globalErrorStream() << "gtkutil::createMenuItem failed to lookup command " << commandName.c_str() << "\n"; 
	}

	return GTK_MENU_ITEM(menuItem);
}

/* greebo: Create a check menuitem under the given <menu> and connect it to the given <command> name
 */
GtkMenuItem* createCheckMenuItemWithMnemonic(GtkMenu* menu, 
											 const std::string& caption, 
											 const std::string& commandName,
											 const std::string& iconName) 
{

	GtkWidget* menuItem = NULL;

	IEventPtr event = GlobalEventManager().findEvent(commandName);

	if (!event->empty()) {
		// Retrieve an acclerator string formatted for a menu
		const std::string accelText = GlobalEventManager().getAcceleratorStr(event, true);
		
		//menuItem = gtkutil::TextMenuItemToggle(caption);
		menuItem = gtkutil::TextMenuItemAccelerator(caption, 
													accelText,
													iconName,
													true);
	
		gtk_widget_show_all(GTK_WIDGET(menuItem));
		
		// Add the menu item to the container
		gtk_container_add(GTK_CONTAINER(menu), GTK_WIDGET(menuItem));
		
		event->connectWidget(GTK_WIDGET(menuItem));
	}
	else {
		globalErrorStream() << "gtkutil::createMenuItem failed to lookup command " << commandName.c_str() << "\n"; 
	}

	return GTK_MENU_ITEM(menuItem);
}

/* Adds a separator to the given menu
 */
GtkMenuItem* createSeparatorMenuItem(GtkMenu* menu) {
	GtkWidget* separator = gtkutil::SeparatorMenuItem();
	gtk_widget_show(separator);
	
	// Add the menu item to the container
	gtk_container_add(GTK_CONTAINER(menu), GTK_WIDGET(separator));
	
	return GTK_MENU_ITEM(separator);
}

// =============================================================================
// File dialog

bool color_dialog (GtkWidget *parent, Vector3& color, const char* title)
{
  GtkWidget* dlg;
  double clr[3];
  ModalDialog dialog;

  clr[0] = color[0];
  clr[1] = color[1];
  clr[2] = color[2];

  dlg = gtk_color_selection_dialog_new (title);
  gtk_color_selection_set_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (dlg)->colorsel), clr);
  g_signal_connect(G_OBJECT(dlg), "delete_event", G_CALLBACK(dialog_delete_callback), &dialog);
  g_signal_connect(G_OBJECT(GTK_COLOR_SELECTION_DIALOG(dlg)->ok_button), "clicked", G_CALLBACK(dialog_button_ok), &dialog);
  g_signal_connect(G_OBJECT(GTK_COLOR_SELECTION_DIALOG(dlg)->cancel_button), "clicked", G_CALLBACK(dialog_button_cancel), &dialog);

  if (parent != 0)
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (parent));

  bool ok = modal_dialog_show(GTK_WINDOW(dlg), dialog) == eIDOK;
  if(ok)
  {
    GdkColor gdkcolor;
    gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (dlg)->colorsel), &gdkcolor);
    clr[0] = gdkcolor.red / 65535.0;
    clr[1] = gdkcolor.green / 65535.0;
    clr[2] = gdkcolor.blue / 65535.0;

    color[0] = (float)clr[0];
    color[1] = (float)clr[1];
    color[2] = (float)clr[2];
  }

  gtk_widget_destroy(dlg);

  return ok;
}

void button_clicked_entry_browse_file(GtkWidget* widget, GtkEntry* entry)
{
  std::string filename = file_dialog(gtk_widget_get_toplevel(widget), TRUE, "Choose File", gtk_entry_get_text(entry));
  
  if(!filename.empty())
  {
    gtk_entry_set_text(entry, filename.c_str());
  }
}

void button_clicked_entry_browse_directory(GtkWidget* widget, GtkEntry* entry)
{
  const char* text = gtk_entry_get_text(entry);
  char *dir = dir_dialog(gtk_widget_get_toplevel(widget), "Choose Directory", path_is_absolute(text) ? text : "" );
  
  if(dir != 0)
  {
    gchar* converted = g_filename_to_utf8(dir, -1, 0, 0, 0);
    gtk_entry_set_text(entry, converted);
    g_free(dir);
    g_free(converted);
  }
}


