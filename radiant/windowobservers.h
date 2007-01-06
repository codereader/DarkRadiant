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

#if !defined(INCLUDED_WINDOWOBSERVERS_H)
#define INCLUDED_WINDOWOBSERVERS_H

#include "windowobserver.h"

#include <gdk/gdktypes.h>

class WindowObserver;
void GlobalWindowObservers_add(WindowObserver* observer);
void GlobalWindowObservers_remove(WindowObserver* observer);
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkWindow GtkWindow;
void GlobalWindowObservers_connectWidget(GtkWidget* widget);
void GlobalWindowObservers_connectTopLevel(GtkWindow* window);

/* greebo: This translates the modifier information from an GDKEvent event->state
 * into the constants defined in include/windowobserver.h */
inline ModifierFlags modifiers_for_state(unsigned int state) {
	ModifierFlags modifiers = c_modifierNone;
	
	if(state & GDK_SHIFT_MASK) {
		modifiers |= c_modifierShift;
	}
	
  	if(state & GDK_CONTROL_MASK) {
  		modifiers |= c_modifierControl;
  	}
  	
	if(state & GDK_MOD1_MASK) {
		modifiers |= c_modifierAlt;
	}
	
	return modifiers;
}

#endif
