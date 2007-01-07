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

#include "commands.h"

#include "debugging/debugging.h"
#include "warnings.h"

#include <map>
#include "string/string.h"
#include "versionlib.h"
#include "gtkutil/accelerator.h"

typedef std::pair<Accelerator, bool> ShortcutValue; // accelerator, isRegistered
typedef std::map<CopiedString, ShortcutValue> Shortcuts;

void Shortcuts_foreach(Shortcuts& shortcuts, CommandVisitor& visitor)
{
  for(Shortcuts::iterator i = shortcuts.begin(); i != shortcuts.end(); ++i)
  {
    visitor.visit((*i).first.c_str(), (*i).second.first);
  }
}

Shortcuts g_shortcuts;

void GlobalShortcuts_foreach(CommandVisitor& visitor)
{
  Shortcuts_foreach(g_shortcuts, visitor);
}

#include <cctype>

#include <gtk/gtkbox.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkcellrenderertext.h>

#include "gtkutil/dialog.h"
#include "mainframe.h"

#include "stream/textfilestream.h"
#include "stream/stringstream.h"


struct command_list_dialog_t : public ModalDialog
{
  command_list_dialog_t()
    : m_close_button(*this, eIDCANCEL)
  {
  }
  ModalDialogButton m_close_button;
};

void DoCommandListDlg()
{
  command_list_dialog_t dialog;

  GtkWindow* window = create_modal_dialog_window(MainFrame_getWindow(), "Mapped Commands", dialog, -1, 400);

  GtkAccelGroup* accel = gtk_accel_group_new();
  gtk_window_add_accel_group(window, accel);

  GtkHBox* hbox = create_dialog_hbox(4, 4);
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(hbox));

  {
    GtkScrolledWindow* scr = create_scrolled_window(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(scr), TRUE, TRUE, 0);

    {
      GtkListStore* store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

      GtkWidget* view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

      {
        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes("Command", renderer, "text", 0, 0);
        gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
      }

      {
        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes("Key", renderer, "text", 1, 0);
        gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
      }

      gtk_widget_show(view);
      gtk_container_add(GTK_CONTAINER (scr), view);

      {
        // Initialize dialog
        StringOutputStream path(256);
        path << SettingsPath_get() << "commandlist.txt";
        globalOutputStream() << "Writing the command list to " << path.c_str() << "\n";
        class BuildCommandList : public CommandVisitor
        {
          TextFileOutputStream m_commandList;
          GtkListStore* m_store;
        public:
          BuildCommandList(const char* filename, GtkListStore* store) : m_commandList(filename), m_store(store)
          {
          }
          void visit(const char* name, Accelerator& accelerator)
          {
            StringOutputStream modifiers;
            modifiers << accelerator;

            {
              GtkTreeIter iter;
              gtk_list_store_append(m_store, &iter);
              gtk_list_store_set(m_store, &iter, 0, name, 1, modifiers.c_str(), -1);
            }
 
            if(!m_commandList.failed())
            {
              m_commandList << makeLeftJustified(name, 25) << " " << modifiers.c_str() << '\n';
            }
          }
        } visitor(path.c_str(), store);

        GlobalShortcuts_foreach(visitor);
      }
    
      g_object_unref(G_OBJECT(store));
    }
  }

  GtkVBox* vbox = create_dialog_vbox(4);
  gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(vbox), FALSE, FALSE, 0);
  {
    GtkButton* button = create_modal_dialog_button("Close", dialog.m_close_button);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(button), FALSE, FALSE, 0);
    widget_make_default(GTK_WIDGET(button));
    gtk_widget_grab_focus(GTK_WIDGET(button));
    gtk_widget_add_accelerator(GTK_WIDGET(button), "clicked", accel, GDK_Return, (GdkModifierType)0, (GtkAccelFlags)0);
    gtk_widget_add_accelerator(GTK_WIDGET(button), "clicked", accel, GDK_Escape, (GdkModifierType)0, (GtkAccelFlags)0);
  }

  modal_dialog_show(window, dialog);
  gtk_widget_destroy(GTK_WIDGET(window));
}
