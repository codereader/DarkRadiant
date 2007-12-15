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

#if !defined(INCLUDED_GTKUTIL_GLWIDGET_H)
#define INCLUDED_GTKUTIL_GLWIDGET_H

// Forward declarations
typedef struct _GdkGLConfig GdkGLConfig;
typedef struct _GtkWidget GtkWidget;
typedef int    gint;
typedef gint   gboolean;

namespace gtkutil {

class GLWidget {
	
	// The actual widget, a GTK drawing area
	GtkWidget* _widget;
	
	// TRUE, if this GL widget has depth-buffering enabled 
	bool _zBuffer;
	
	// The (singleton) widget holding the context
	static GtkWidget* _shared;
	
	// Holds the number of realised GL widgets
	static int _realisedWidgets;

public:
	// Constructor, pass TRUE to enable depth-buffering
	GLWidget(bool zBuffer);
	
	// Operator cast to GtkWidget*, for packing into parent containers
	operator GtkWidget*() const;
	
	// Switches the GL context to the given widget
	static bool makeCurrent(GtkWidget* widget);
	static void swapBuffers(GtkWidget* widget);
	
private:
	// As soon as the widget is packed into a parent, this callback is invoked
	// and enables the GL drawing for this widget
	static gboolean onHierarchyChanged(GtkWidget* widget, GtkWidget* previous_toplevel, GLWidget* self);
	
	// Called when the GTK drawing area is realised/unrealised 
	static gint onRealise(GtkWidget* widget, GLWidget* self);
	static gint onUnRealise(GtkWidget* widget, GLWidget* self);
	
	// Acquires a GDK GL config structure with or without depth
	static GdkGLConfig* createGLConfigWithDepth();
	static GdkGLConfig* createGLConfig();
};

} // namespace gtkutil

#endif
