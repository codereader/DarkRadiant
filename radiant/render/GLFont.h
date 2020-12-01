#pragma once

#include <memory>
#include <FTGL/ftgl.h>
#include "igl.h"

namespace gl
{

class GLFont :
    public IGLFont
{
private:
	float _lineHeight;
	FTGL::FTGLfont* _ftglFont;

public:
	// the constructor will allocate the FTGL font
	GLFont(Style style, unsigned int size);

	// Destructor frees the FTGL object again
	~GLFont();

    float getLineHeight() const override;

    void drawString(const std::string& string) override;
};

} // namespace
