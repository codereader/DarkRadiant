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

#include "glfont.h"

#include "igl.h"
#include <gtkmm/gl/widget.h>
#include <gdkmm/gl/font.h>

GLFont GLFont::create(const char* fontString)
{
	GLuint font_list_base = glGenLists(256);
	int font_height = 0;

	Pango::FontDescription fontDesc(fontString);

	Glib::RefPtr<Pango::Font> font = Gdk::GL::Font::use_pango_font(fontDesc, 0, 256, font_list_base);

	if (font)
	{
		Pango::FontMetrics fontMetrics = font->get_metrics();

		font_height = fontMetrics.get_ascent() + fontMetrics.get_descent();
		font_height = PANGO_PIXELS(font_height);
	}

	return GLFont(font_list_base, font_height);
}

void GLFont::release(GLFont& font)
{
	glDeleteLists(font.getDisplayList(), 256);
	font.clear();
}
