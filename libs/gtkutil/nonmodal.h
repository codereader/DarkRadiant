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

#if !defined(INCLUDED_GTKUTIL_NONMODAL_H)
#define INCLUDED_GTKUTIL_NONMODAL_H

#include <gtk/gtkwidget.h>

inline gboolean escape_clear_focus_widget(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
  if(event->keyval == GDK_Escape)
  {
    gtk_window_set_focus(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(widget))), NULL);
    return TRUE;
  }
  return FALSE;
}

inline void widget_connect_escape_clear_focus_widget(GtkWidget* widget)
{
  g_signal_connect(G_OBJECT(widget), "key_press_event", G_CALLBACK(escape_clear_focus_widget), 0);
}

#endif
