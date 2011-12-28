#include "GLFont.h"

#include "igl.h"
#include "imodule.h"
#include <gtkmm/gl/widget.h>
#include <gdkmm/gl/font.h>
#include <iostream>
#include <fontconfig/fontconfig.h>

namespace gtkutil
{

GLFont::GLFont(Style style, unsigned int size) :
	_pixelHeight(0),
	_ftglFont(NULL)
{
#ifndef WIN32
	const std::string fontName = style == FONT_SANS ? "Sans" : "Mono";

	// get filename for requested font
	FcPattern* pattern = FcNameParse(reinterpret_cast<const FcChar8*>(fontName.c_str()));

	// No font substitution in Win32
	FcConfigSubstituteWithPat(NULL, pattern, NULL, FcMatchPattern);
	FcDefaultSubstitute(pattern);
	
	FcResult res;
	pattern = FcFontMatch(NULL,pattern, &res);

	if (res != FcResultMatch)
	{
		const char *fc_error;

		switch(res)
		{
			case FcResultNoMatch:
				fc_error = "No Match found";
				break;
			case FcResultTypeMismatch:
				fc_error = "Result type mismatch";
				break;
			case FcResultNoId:
				fc_error = "Invalid id value specified";
				break;
			default:
				fc_error = "unknown error";
		}

		g_critical("The requested font \"%s\" could not be matched. FcFontMatch returned: %s", fontName, fc_error);
	}


	FcChar8* fcstring;
	FcPatternGetString(pattern, "file", 0, &fcstring);

	std::string fontpath(reinterpret_cast<const char*>(fcstring));

	FcPatternDestroy(pattern);

#else // Win32

	std::string fontpath = module::GlobalModuleRegistry()
                           .getApplicationContext()
                           .getRuntimeDataPath()
                           + "ui/fonts/";

	fontpath += style == FONT_SANS ? "FreeSans.ttf" : "FreeMono.ttf";

#endif

	_ftglFont = FTGL::ftglCreatePixmapFont(fontpath.c_str());

	if (_ftglFont)
	{
		ftglSetFontFaceSize(_ftglFont,size,0);
		_pixelHeight = FTGL::ftglGetFontAscender(_ftglFont)+FTGL::ftglGetFontDescender(_ftglFont);
		_pixelHeight = PANGO_PIXELS(_pixelHeight);
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
