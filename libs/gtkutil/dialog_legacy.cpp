/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

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

#include "dialog.h"
#include "EntryAbortedException.h"

#include <gtk/gtkmain.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkframe.h>
#include <gtk/gtktable.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtklabel.h>
#include <gtk/gtk.h>

#include "window.h"

#include <string>

GtkLabel* DialogLabel_new(const char* name)
{
  GtkLabel* label = GTK_LABEL(gtk_label_new(name));
  gtk_widget_show(GTK_WIDGET(label));
  gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
  gtk_label_set_justify(label, GTK_JUSTIFY_LEFT);

  return label;
}

GtkTable* DialogRow_new(const char* name, GtkWidget* widget)
{
  GtkTable* table = GTK_TABLE(gtk_table_new(1, 3, TRUE));
  gtk_widget_show(GTK_WIDGET(table));

  gtk_table_set_col_spacings(table, 4);
  gtk_table_set_row_spacings(table, 0);

  gtk_table_attach(table, GTK_WIDGET(DialogLabel_new(name)), 0, 1, 0, 1,
      (GtkAttachOptions) (GTK_EXPAND|GTK_FILL),
      (GtkAttachOptions) (0), 0, 0);

  gtk_table_attach(table, widget, 1, 3, 0, 1,
      (GtkAttachOptions) (GTK_EXPAND|GTK_FILL),
      (GtkAttachOptions) (0), 0, 0);

  return table;
}

void DialogVBox_packRow(GtkVBox* vbox, GtkWidget* row)
{
  gtk_box_pack_start(GTK_BOX(vbox), row, FALSE, FALSE, 0);
}

/* gtkutil namespace */

namespace gtkutil {
	
// Display a Gtk Error dialog

void errorDialog(const std::string& errorText, GtkWindow* mainFrame) {
	GtkWidget* dialog = 
		gtk_message_dialog_new_with_markup (mainFrame,
                       				        GTK_DIALOG_DESTROY_WITH_PARENT,
				                            GTK_MESSAGE_ERROR,
                   			                GTK_BUTTONS_CLOSE,
                               				errorText.c_str());
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

// Display a fatal Gtk Error dialog

void fatalErrorDialog(const std::string& errorText, GtkWindow* mainFrame) {
	errorDialog(errorText, mainFrame);
	abort();	
}
	
    
// Display a text entry dialog

const std::string textEntryDialog(const std::string& title, 
								  const std::string& prompt,
								  const std::string& defaultText,
								  GtkWindow* mainFrame) 
{
    GtkWidget* dialog = gtk_dialog_new_with_buttons(title.c_str(),
                                                    mainFrame,
                                                    static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                                                    GTK_STOCK_CANCEL,
                                                    GTK_RESPONSE_REJECT,
                                                    GTK_STOCK_OK,
                                                    GTK_RESPONSE_ACCEPT,
                                                    NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

    // Pack the label and entry widgets into the dialog
    
    GtkWidget* hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
    gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(prompt.c_str()), FALSE, FALSE, 0);

    GtkWidget* entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), defaultText.c_str());
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 3);

    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);

    // Display dialog and get user response
    gtk_widget_show_all(dialog);
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    std::string text = gtk_entry_get_text(GTK_ENTRY(entry));
    gtk_widget_destroy(dialog);

    if (result == GTK_RESPONSE_ACCEPT)
        return text;
    else
        throw EntryAbortedException("textEntryDialog(): dialog cancelled");
}
    
} // namespace gtkutil
