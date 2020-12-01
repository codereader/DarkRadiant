#include "GLFont.h"

#include "itextstream.h"
#include "igl.h"
#include "imodule.h"
#include <iostream>

namespace gl
{

GLFont::GLFont(Style style, unsigned int size) :
    _lineHeight(0),
	_ftglFont(nullptr)
{
    // Load the locally-provided TTF font file
	std::string fontpath = module::GlobalModuleRegistry()
                           .getApplicationContext()
                           .getRuntimeDataPath()
                           + "ui/fonts/";

	fontpath += style == Style::Sans ? "FreeSans.ttf" : "FreeMono.ttf";

	_ftglFont = FTGL::ftglCreatePixmapFont(fontpath.c_str());

	if (_ftglFont)
	{
        FTGL::ftglSetFontFaceSize(_ftglFont, size, 0);
        _lineHeight = FTGL::ftglGetFontLineHeight(_ftglFont);
	}
	else
	{
		rError() << "Failed to create FTGLPixmapFont" << std::endl;
	}
}

GLFont::~GLFont()
{
	if (_ftglFont)
	{
		FTGL::ftglDestroyFont(_ftglFont);
		_ftglFont = nullptr;
	}
}

float GLFont::getLineHeight() const
{
    return _lineHeight;
}

void GLFont::drawString(const std::string& string)
{
    FTGL::ftglRenderFont(_ftglFont, string.c_str(), FTGL::RENDER_ALL);
}

} // namespace
