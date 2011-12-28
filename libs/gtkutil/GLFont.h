#pragma once

#include <boost/shared_ptr.hpp>
#include <FTGL/ftgl.h>

namespace gtkutil
{

class GLFont
{
private:
	int _pixelHeight;
	FTGL::FTGLfont* _ftglFont;	

public:
	enum Style
	{
		FONT_SANS,	// free sans
		FONT_MONO,	// free mono
	};

	// the constructor will allocate the FTGL font
	GLFont(Style style, unsigned int size);

	// Destructor frees the FTGL object again
	~GLFont();

	FTGL::FTGLfont* getFtglFont()
	{
		return _ftglFont;
	}

	int getPixelHeight() const
	{
		return _pixelHeight;
	}
};
typedef boost::shared_ptr<GLFont> GLFontPtr;

} // namespace
