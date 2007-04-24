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

#if !defined(INCLUDED_GTKUTIL_WINDOW_H)
#define INCLUDED_GTKUTIL_WINDOW_H

#include <gtk/gtkwindow.h>

#include "debugging/debugging.h"
#include "generic/callback.h"
#include "widget.h"

inline gboolean window_focus_in_clear_focus_widget(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
  gtk_window_set_focus(GTK_WINDOW(widget), NULL);
  return FALSE;
}

inline guint window_connect_focus_in_clear_focus_widget(GtkWindow* window)
{
  return g_signal_connect(G_OBJECT(window), "focus_in_event", G_CALLBACK(window_focus_in_clear_focus_widget), NULL);
}


unsigned int connect_floating(GtkWindow* main_window, GtkWindow* floating);
GtkWindow* create_floating_window(const char* title, GtkWindow* parent);
void destroy_floating_window(GtkWindow* window);

void window_remove_minmax(GtkWindow* window);

typedef struct _GtkScrolledWindow GtkScrolledWindow;
GtkScrolledWindow* create_scrolled_window(GtkPolicyType hscrollbar_policy, GtkPolicyType vscrollbar_policy, int border = 0);

#endif
