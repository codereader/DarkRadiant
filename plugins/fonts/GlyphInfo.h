#ifndef _GLYPH_INFO_H_
#define _GLYPH_INFO_H_

#include <boost/shared_ptr.hpp>

namespace fonts
{

// Container-class for Glyphs.
struct GlyphInfo
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
	int textureIndex;
};
typedef boost::shared_ptr<GlyphInfo> GlyphInfoPtr;

} // namespace fonts

#endif /* _GLYPH_INFO_H_ */
