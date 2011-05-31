#include "GLFont.h"

#include "igl.h"
#include <gtkmm/gl/widget.h>
#include <gdkmm/gl/font.h>

namespace gtkutil
{

GLFont::GLFont(const char* fontName)
{
	_displayList = glGenLists(256);
	_pixelHeight = 0;

	Pango::FontDescription fontDesc(fontName);

	Glib::RefPtr<Pango::Font> font = Gdk::GL::Font::use_pango_font(fontDesc, 0, 256, _displayList);

	if (font)
	{
		Pango::FontMetrics fontMetrics = font->get_metrics();

		_pixelHeight = fontMetrics.get_ascent() + fontMetrics.get_descent();
		_pixelHeight = PANGO_PIXELS(_pixelHeight);
	}
}

GLFont::~GLFont()
{
	glDeleteLists(_displayList, 256);
}

} // namespace
