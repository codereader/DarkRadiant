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

#if !defined(INCLUDED_GTKUTIL_CURSOR_H)
#define INCLUDED_GTKUTIL_CURSOR_H

#include <glib/gmain.h>
#include <gdk/gdkevents.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>

#include "debugging/debugging.h"

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkWindow GtkWindow;

void Sys_GetCursorPos(GtkWindow* window, int *x, int *y);
void Sys_SetCursorPos(GtkWindow* window, int x, int y);

class DeferredMotionDelta
{
  int m_delta_x;
  int m_delta_y;
  guint m_motion_handler;
  typedef void (*MotionDeltaFunction)(int x, int y, void* data);
  MotionDeltaFunction m_function;
  void* m_data;

  static gboolean deferred_motion(gpointer data)
  {
    reinterpret_cast<DeferredMotionDelta*>(data)->m_function(
      reinterpret_cast<DeferredMotionDelta*>(data)->m_delta_x,
      reinterpret_cast<DeferredMotionDelta*>(data)->m_delta_y,
      reinterpret_cast<DeferredMotionDelta*>(data)->m_data
    );
    reinterpret_cast<DeferredMotionDelta*>(data)->m_motion_handler = 0;
    reinterpret_cast<DeferredMotionDelta*>(data)->m_delta_x = 0;
    reinterpret_cast<DeferredMotionDelta*>(data)->m_delta_y = 0;
    return FALSE;
  }
public:
  DeferredMotionDelta(MotionDeltaFunction function, void* data) : m_delta_x(0), m_delta_y(0), m_motion_handler(0), m_function(function), m_data(data)
  {
  }
  void flush()
  {
    if(m_motion_handler != 0)
    {
      g_source_remove(m_motion_handler);
      deferred_motion(this);
    }
  }
  void motion_delta(int x, int y, unsigned int state)
  {
    m_delta_x += x;
    m_delta_y += y;
    if(m_motion_handler == 0)
    {
      m_motion_handler = g_idle_add(deferred_motion, this);
    }
  }
};



#endif
