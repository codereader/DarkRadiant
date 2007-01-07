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

#include "accelerator.h"

#include "debugging/debugging.h"

#include <map>
#include <set>
#include <gtk/gtkwindow.h>
#include <gtk/gtkaccelgroup.h>

#include "generic/callback.h"
#include "generic/bitfield.h"
#include "string/string.h"

#include "pointer.h"
#include "closure.h"

#include <gdk/gdkkeysyms.h>



struct SKeyInfo
{
  const char* m_strName;
  unsigned int m_nVKKey;
};

SKeyInfo g_Keys[] =
{
  {"Space", GDK_space},
  {"Backspace", GDK_BackSpace},
  {"Escape", GDK_Escape},
  {"End", GDK_End},
  {"Insert", GDK_Insert},
  {"Delete", GDK_Delete},
  {"PageUp", GDK_Prior},
  {"PageDown", GDK_Next},
  {"Up", GDK_Up},
  {"Down", GDK_Down},
  {"Left", GDK_Left},
  {"Right", GDK_Right},
  {"F1", GDK_F1},
  {"F2", GDK_F2},
  {"F3", GDK_F3},
  {"F4", GDK_F4},
  {"F5", GDK_F5},
  {"F6", GDK_F6},
  {"F7", GDK_F7},
  {"F8", GDK_F8},
  {"F9", GDK_F9},
  {"F10", GDK_F10},
  {"F11", GDK_F11},
  {"F12", GDK_F12},
  {"Tab", GDK_Tab},
  {"Return", GDK_Return},                           
  {"Comma", GDK_comma},
  {"Period", GDK_period},
  {"Plus", GDK_KP_Add},
  {"Multiply", GDK_multiply},
  {"Minus", GDK_KP_Subtract},
  {"NumPad0", GDK_KP_0},
  {"NumPad1", GDK_KP_1},
  {"NumPad2", GDK_KP_2},
  {"NumPad3", GDK_KP_3},
  {"NumPad4", GDK_KP_4},
  {"NumPad5", GDK_KP_5},
  {"NumPad6", GDK_KP_6},
  {"NumPad7", GDK_KP_7},
  {"NumPad8", GDK_KP_8},
  {"NumPad9", GDK_KP_9},
  {"[", 219},
  {"]", 221},
  {"\\", 220},
  {"Home", GDK_Home}
};

int g_nKeyCount = sizeof(g_Keys) / sizeof(SKeyInfo);

const char* global_keys_find(unsigned int key)
{
  for(int i = 0; i < g_nKeyCount; ++i)
  {
    if(g_Keys[i].m_nVKKey == key)
    {
      return g_Keys[i].m_strName;
    }
  }
  return "";
}

unsigned int global_keys_find(const char* name)
{
  for(int i = 0; i < g_nKeyCount; ++i)
  {
    if(string_equal_nocase(g_Keys[i].m_strName, name))
    {
      return g_Keys[i].m_nVKKey;
    }
  }
  return 0;
}

void accelerator_write(const Accelerator& accelerator, TextOutputStream& ostream)
{
  if(accelerator.modifiers & GDK_SHIFT_MASK)
  {
    ostream << "Shift + ";
  }
  if(accelerator.modifiers & GDK_MOD1_MASK)
  {
    ostream << "Alt + ";
  }
  if(accelerator.modifiers & GDK_CONTROL_MASK)
  {
    ostream << "Control + ";
  }

  const char* keyName = global_keys_find(accelerator.key);
  if(!string_empty(keyName))
  {
    ostream << keyName;
  }
  else
  {
    ostream << static_cast<char>(accelerator.key);
  }
}

typedef std::map<Accelerator, Callback> AcceleratorMap;
typedef std::set<Accelerator> AcceleratorSet;

bool accelerator_map_insert(AcceleratorMap& acceleratorMap, Accelerator accelerator, const Callback& callback)
{
  if(accelerator.key != 0)
  {
    return acceleratorMap.insert(AcceleratorMap::value_type(accelerator, callback)).second;
  }
  return true;
}

bool accelerator_map_erase(AcceleratorMap& acceleratorMap, Accelerator accelerator)
{
  if(accelerator.key != 0)
  {
    AcceleratorMap::iterator i = acceleratorMap.find(accelerator);
    if(i == acceleratorMap.end())
    {
      return false;
    }
    acceleratorMap.erase(i);
  }
  return true;
}

Accelerator accelerator_for_event_key(guint keyval, guint state)
{
  keyval = gdk_keyval_to_upper(keyval);
  if(keyval == GDK_ISO_Left_Tab)
    keyval = GDK_Tab;
  return Accelerator(keyval, (GdkModifierType)(state & gtk_accelerator_get_default_mod_mask()));
}

bool AcceleratorMap_activate(const AcceleratorMap& acceleratorMap, const Accelerator& accelerator)
{
  AcceleratorMap::const_iterator i = acceleratorMap.find(accelerator);
  if(i != acceleratorMap.end())
  {
    (*i).second();
    return true;
  }

  return false;
}

AcceleratorMap g_special_accelerators;


namespace MouseButton
{
  enum 
  {
    Left = 1 << 0,
    Right = 1 << 1,
    Middle = 1 << 2,
  };
}

typedef unsigned int ButtonMask;

void print_buttons(ButtonMask mask)
{
  globalOutputStream() << "button state: ";
  if((mask & MouseButton::Left) != 0)
  {
    globalOutputStream() << "Left ";
  }
  if((mask & MouseButton::Right) != 0)
  {
    globalOutputStream() << "Right ";
  }
  if((mask & MouseButton::Middle) != 0)
  {
    globalOutputStream() << "Middle ";
  }
  globalOutputStream() << "\n";
}

ButtonMask ButtonMask_for_event_button(guint button)
{
  switch(button)
  {
  case 1:
    return MouseButton::Left;
  case 2:
    return MouseButton::Middle;
  case 3:
    return MouseButton::Right;
  }
  return 0;
}

bool window_has_accel(GtkWindow* toplevel)
{
  return g_slist_length(gtk_accel_groups_from_object(G_OBJECT(toplevel))) != 0;
}

namespace
{
  bool g_accel_enabled = true;
}

bool global_accel_enabled()
{
  return g_accel_enabled;
}


GClosure* accel_group_add_accelerator(GtkAccelGroup* group, Accelerator accelerator, const Callback& callback);
void accel_group_remove_accelerator(GtkAccelGroup* group, Accelerator accelerator);

AcceleratorMap g_queuedAcceleratorsAdd;
AcceleratorSet g_queuedAcceleratorsRemove;

void globalQueuedAccelerators_add(Accelerator accelerator, const Callback& callback)
{
  if(!g_queuedAcceleratorsAdd.insert(AcceleratorMap::value_type(accelerator, callback)).second)
  {
    globalErrorStream() << "globalQueuedAccelerators_add: accelerator already queued: " << accelerator << "\n";
  }
}

void globalQueuedAccelerators_remove(Accelerator accelerator)
{
  if(g_queuedAcceleratorsAdd.erase(accelerator) == 0)
  {
    if(!g_queuedAcceleratorsRemove.insert(accelerator).second)
    {
      globalErrorStream() << "globalQueuedAccelerators_remove: accelerator already queued: " << accelerator << "\n";
    }
  }
}

void globalQueuedAccelerators_commit()
{
  for(AcceleratorSet::const_iterator i = g_queuedAcceleratorsRemove.begin(); i != g_queuedAcceleratorsRemove.end(); ++i)
  {
    //globalOutputStream() << "removing: " << (*i).first << "\n";
    accel_group_remove_accelerator(global_accel, *i);
  }
  g_queuedAcceleratorsRemove.clear();
  for(AcceleratorMap::const_iterator i = g_queuedAcceleratorsAdd.begin(); i != g_queuedAcceleratorsAdd.end(); ++i)
  {
    //globalOutputStream() << "adding: " << (*i).first << "\n";
    accel_group_add_accelerator(global_accel, (*i).first, (*i).second);
  }
  g_queuedAcceleratorsAdd.clear();
}

void accel_group_test(GtkWindow* toplevel, GtkAccelGroup* accel)
{
  guint n_entries;
  gtk_accel_group_query(accel, '4', (GdkModifierType)0, &n_entries);
  globalOutputStream() << "grid4: " << n_entries << "\n";
  globalOutputStream() << "toplevel accelgroups: " << g_slist_length(gtk_accel_groups_from_object(G_OBJECT(toplevel))) << "\n";
}

typedef std::set<GtkWindow*> WindowSet;
WindowSet g_accel_windows;

bool Buttons_press(ButtonMask& buttons, guint button, guint state)
{
  if(buttons == 0 && bitfield_enable(buttons, ButtonMask_for_event_button(button)) != 0)
  {
    ASSERT_MESSAGE(g_accel_enabled, "Buttons_press: accelerators not enabled");
    g_accel_enabled = false;
    for(WindowSet::iterator i = g_accel_windows.begin(); i != g_accel_windows.end(); ++i)
    {
      GtkWindow* toplevel = *i;
      ASSERT_MESSAGE(window_has_accel(toplevel), "ERROR");
      ASSERT_MESSAGE(GTK_WIDGET_TOPLEVEL(toplevel), "disabling accel for non-toplevel window");
      //gtk_window_remove_accel_group(toplevel,  global_accel);
#if 0
      globalOutputStream() << reinterpret_cast<unsigned int>(toplevel) << ": disabled global accelerators\n";
#endif
#if 0
      accel_group_test(toplevel, global_accel);
#endif
    }
  }
  buttons = bitfield_enable(buttons, ButtonMask_for_event_button(button));
#if 0
  globalOutputStream() << "Buttons_press: ";
  print_buttons(buttons);
#endif
  return false;
}

bool Buttons_release(ButtonMask& buttons, guint button, guint state)
{
  if(buttons != 0 && bitfield_disable(buttons, ButtonMask_for_event_button(button)) == 0)
  {
    ASSERT_MESSAGE(!g_accel_enabled, "Buttons_release: accelerators are enabled");
    g_accel_enabled = true;
    for(WindowSet::iterator i = g_accel_windows.begin(); i != g_accel_windows.end(); ++i)
    {
      GtkWindow* toplevel = *i;
      ASSERT_MESSAGE(!window_has_accel(toplevel), "ERROR");
      ASSERT_MESSAGE(GTK_WIDGET_TOPLEVEL(toplevel), "enabling accel for non-toplevel window");
      //gtk_window_add_accel_group(toplevel, global_accel);
#if 0
      globalOutputStream() << reinterpret_cast<unsigned int>(toplevel) << ": enabled global accelerators\n";
#endif
#if 0
      accel_group_test(toplevel, global_accel);
#endif
    }
    globalQueuedAccelerators_commit();
  }
  buttons = bitfield_disable(buttons, ButtonMask_for_event_button(button));
#if 0
  globalOutputStream() << "Buttons_release: ";
  print_buttons(buttons);
#endif
  return false;
}

bool Buttons_releaseAll(ButtonMask& buttons)
{
  Buttons_release(buttons, MouseButton::Left | MouseButton::Middle | MouseButton::Right, 0);
  return false;
}

struct PressedButtons
{
  ButtonMask buttons;

  PressedButtons() : buttons(0)
  {
  }
};

gboolean PressedButtons_button_press(GtkWidget* widget, GdkEventButton* event, PressedButtons* pressed)
{
  if(event->type == GDK_BUTTON_PRESS)
  {
    return Buttons_press(pressed->buttons, event->button, event->state);
  }
  return FALSE;
}

gboolean PressedButtons_button_release(GtkWidget* widget, GdkEventButton* event, PressedButtons* pressed)
{
  if(event->type == GDK_BUTTON_RELEASE)
  {
    return Buttons_release(pressed->buttons, event->button, event->state);
  }
  return FALSE;
}

gboolean PressedButtons_focus_out(GtkWidget* widget, GdkEventFocus* event, PressedButtons* pressed)
{
  Buttons_releaseAll(pressed->buttons);
  return FALSE;
}

void PressedButtons_connect(PressedButtons& pressedButtons, GtkWidget* widget)
{
  g_signal_connect(G_OBJECT(widget), "button_press_event", G_CALLBACK(PressedButtons_button_press), &pressedButtons);
  g_signal_connect(G_OBJECT(widget), "button_release_event", G_CALLBACK(PressedButtons_button_release), &pressedButtons);
  g_signal_connect(G_OBJECT(widget), "focus_out_event", G_CALLBACK(PressedButtons_focus_out), &pressedButtons);
}

PressedButtons g_pressedButtons;


#include <set>

void special_accelerators_add(Accelerator accelerator, const Callback& callback)
{
  //globalOutputStream() << "special_accelerators_add: " << makeQuoted(accelerator) << "\n";
  if(!accelerator_map_insert(g_special_accelerators, accelerator, callback))
  {
    globalErrorStream() << "special_accelerators_add: already exists: " << makeQuoted(accelerator) << "\n";
  }
}
void special_accelerators_remove(Accelerator accelerator)
{
  //globalOutputStream() << "special_accelerators_remove: " << makeQuoted(accelerator) << "\n";
  if(!accelerator_map_erase(g_special_accelerators, accelerator))
  {
    globalErrorStream() << "special_accelerators_remove: not found: " << makeQuoted(accelerator) << "\n";
  }
}

gboolean accel_closure_callback(GtkAccelGroup* group, GtkWidget* widget, guint key, GdkModifierType modifiers, gpointer data)
{
  (*reinterpret_cast<Callback*>(data))();
  return TRUE;
}

GClosure* accel_group_add_accelerator(GtkAccelGroup* group, Accelerator accelerator, const Callback& callback)
{
  if(accelerator.key != 0 && gtk_accelerator_valid(accelerator.key, accelerator.modifiers))
  {
    //globalOutputStream() << "global_accel_connect: " << makeQuoted(accelerator) << "\n";
    GClosure* closure = create_cclosure(G_CALLBACK(accel_closure_callback), callback);
    gtk_accel_group_connect(group, accelerator.key, accelerator.modifiers, GTK_ACCEL_VISIBLE, closure);
    return closure;
  }
  else
  {
    special_accelerators_add(accelerator, callback);
    return 0;
  }
}

void accel_group_remove_accelerator(GtkAccelGroup* group, Accelerator accelerator)
{
  if(accelerator.key != 0 && gtk_accelerator_valid(accelerator.key, accelerator.modifiers))
  {
    //globalOutputStream() << "global_accel_disconnect: " << makeQuoted(accelerator) << "\n";
    gtk_accel_group_disconnect_key(group, accelerator.key, accelerator.modifiers);
  }
  else
  {
    special_accelerators_remove(accelerator);
  }
}

GtkAccelGroup* global_accel = 0;
