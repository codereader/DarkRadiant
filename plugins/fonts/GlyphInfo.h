#ifndef _GLYPH_INFO_H_
#define _GLYPH_INFO_H_

#include "ifonts.h"
#include <boost/shared_ptr.hpp>
#include "ishaders.h"

namespace fonts
{

namespace q3font
{

	// Default values of Quake 3 sourcecode. Don't change!
	const int SHADER_NAME_LENGTH = 32;
	const int FONT_NAME_LENGTH = 64;

	struct Q3GlyphInfo
	{
		int height;       // number of scan lines
		int top;          // top of glyph in buffer
		int bottom;       // bottom of glyph in buffer
		int pitch;        // width for copying
		int xSkip;        // x adjustment
		int imageWidth;   // width of actual image
		int imageHeight;  // height of actual image
		float s;          // x offset in image where glyph starts
		float t;          // y offset in image where glyph starts
		float s2;
		float t2;
		int glyph;		  // handle to the shader with the glyph
		char shaderName[q3font::SHADER_NAME_LENGTH];
	};

	struct Q3FontInfo
	{
		Q3GlyphInfo glyphs[q3font::GLYPH_COUNT_PER_FONT];
		float glyphScale;
		char name[q3font::FONT_NAME_LENGTH];
	};
	typedef boost::shared_ptr<Q3FontInfo> Q3FontInfoPtr;

} // namespace q3font

// Container-class for Glyphs.
class GlyphInfo :
	public IGlyphInfo
{
public:
	// Construct a GlyphInfo from a given Q3GlyphInfo structure
	// as read from a font DAT file
	GlyphInfo(const q3font::Q3GlyphInfo& q3glyph);
};
typedef boost::shared_ptr<GlyphInfo> GlyphInfoPtr;

} // namespace fonts

#endif /* _GLYPH_INFO_H_ */
