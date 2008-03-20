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

#if !defined(INCLUDED_GTKUTIL_WIDGET_H)
#define INCLUDED_GTKUTIL_WIDGET_H

#include <list>
#include <gtk/gtkwidget.h>
#include "generic/callback.h"
#include "warnings.h"
#include "debugging/debugging.h"

inline void widget_set_visible(GtkWidget* widget, bool shown)
{
  if(shown)
  {
    gtk_widget_show(widget);
  }
  else
  {
    gtk_widget_hide(widget);
  }
}

inline bool widget_is_visible(GtkWidget* widget)
{
  return GTK_WIDGET_VISIBLE(widget) != FALSE;
}

inline void widget_toggle_visible(GtkWidget* widget)
{
  widget_set_visible(widget, !widget_is_visible(widget));
}

inline void widget_queue_draw(GtkWidget& widget)
{
  gtk_widget_queue_draw(&widget);
}
typedef ReferenceCaller<GtkWidget, widget_queue_draw> WidgetQueueDrawCaller;


inline void widget_make_default(GtkWidget* widget)
{
  GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(widget);
}

class WidgetFocusPrinter
{
  const char* m_name;

  static gboolean focus_in(GtkWidget *widget, GdkEventFocus *event, WidgetFocusPrinter* self)
  {
    globalOutputStream() << self->m_name << " takes focus\n";
    return FALSE;
  }
  static gboolean focus_out(GtkWidget *widget, GdkEventFocus *event, WidgetFocusPrinter* self)
  {
    globalOutputStream() << self->m_name << " loses focus\n";
    return FALSE;
  }
public:
  WidgetFocusPrinter(const char* name) : m_name(name)
  {
  }
  void connect(GtkWidget* widget)
  {
    g_signal_connect(G_OBJECT(widget), "focus_in_event", G_CALLBACK(focus_in), this);
    g_signal_connect(G_OBJECT(widget), "focus_out_event", G_CALLBACK(focus_out), this);
  }
};

void Widget_connectToggleDependency(GtkWidget* self, GtkWidget* toggleButton);

#endif
