#pragma once

#include <boost/shared_ptr.hpp>

namespace gtkutil
{

typedef unsigned int GLuint;

class GLFont
{
private:
	GLuint _displayList;
	int _pixelHeight;

public:
	// Construct a font using the Pango font name
	// the constructor will allocate the GL display lists
	GLFont(const char* fontName);

	// Destructor frees the GL display list again
	~GLFont();

	GLuint getDisplayList() const
	{
		return _displayList;
	}

	int getPixelHeight() const
	{
		return _pixelHeight;
	}
};
typedef boost::shared_ptr<GLFont> GLFontPtr;

} // namespace
