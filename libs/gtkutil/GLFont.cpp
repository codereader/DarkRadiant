#include "GLFont.h"

#include "igl.h"
#include "imodule.h"
#include <gtkmm/gl/widget.h>
#include <gdkmm/gl/font.h>
#include <iostream>

namespace gtkutil
{

GLFont::GLFont(Style style, unsigned int size) :
	_pixelHeight(0),
	_ftglFont(NULL)
{
    // Load the locally-provided TTF font file
	std::string fontpath = module::GlobalModuleRegistry()
                           .getApplicationContext()
                           .getRuntimeDataPath()
                           + "ui/fonts/";

	fontpath += style == FONT_SANS ? "FreeSans.ttf" : "FreeMono.ttf";

	_ftglFont = FTGL::ftglCreatePixmapFont(fontpath.c_str());

	if (_ftglFont)
	{
		ftglSetFontFaceSize(_ftglFont,size,0);
		_pixelHeight = FTGL::ftglGetFontLineHeight(_ftglFont);
	}
	else
	{
		g_critical("Failed to create FTGLPixmapFont");
	}
}

GLFont::~GLFont()
{
	if (_ftglFont)
	{
		FTGL::ftglDestroyFont(_ftglFont);
		_ftglFont = NULL;
	}
}

} // namespace
