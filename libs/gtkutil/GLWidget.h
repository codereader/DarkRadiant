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

#include <boost/shared_ptr.hpp>
#include <string>
#include <GL/glew.h>
#include <gtkmm/gl/drawingarea.h>

// greebo: Undo the min max macro definitions coming from a windows header
#undef min
#undef max

namespace gtkutil
{

class GLWidget :
	public Gtk::GL::DrawingArea
{
private:
	// TRUE, if this GL widget has depth-buffering enabled 
	bool _zBuffer;

	// (Shared) widget holding the context, managed in the OpenGLModule
	Gtk::Widget* _context;
	
public:

	// Constructor, pass TRUE to enable depth-buffering
    GLWidget(bool zBuffer, const std::string& debugName = std::string());

	~GLWidget();
	
	// Switches the GL context to the given widget
	static bool makeCurrent(Gtk::Widget& widget);
	static void swapBuffers(Gtk::Widget& widget);

	void queueDraw();
	
private:
	// As soon as the widget is packed into a parent, this callback is invoked
	// and enables the GL drawing for this widget
	void onHierarchyChanged(Gtk::Widget* previous_toplevel);
	
	// Called when the GTK drawing area is realised/unrealised 
	void onRealise();
	void onUnRealise();
	
	// Acquires a GDK GL config structure with or without depth
	static Glib::RefPtr<Gdk::GL::Config> createGLConfigWithDepth();
	static Glib::RefPtr<Gdk::GL::Config> createGLConfig();
};
typedef boost::shared_ptr<GLWidget> GLWidgetPtr;

} // namespace gtkutil

#endif
