#include "GlyphInfo.h"

#include "GlyphInfo.h"
#include "os/file.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace fonts
{

GlyphInfo::GlyphInfo(const q3font::Q3GlyphInfo& q3glyph)
{
	height = q3glyph.height;
	top = q3glyph.top;
	bottom = q3glyph.bottom;
	pitch = q3glyph.pitch;
	xSkip = q3glyph.xSkip;
	imageWidth = q3glyph.imageWidth;
	imageHeight = q3glyph.imageHeight;
	s = q3glyph.s;
	t = q3glyph.t;
	s2 = q3glyph.s2;
	t2 = q3glyph.t2;

	texture = q3glyph.shaderName;
	boost::algorithm::to_lower(texture);

	// Cut off fonts/
	boost::algorithm::replace_first(texture, "fonts/", "");

	// Cut off file extension
	texture = texture.substr(0, texture.rfind('.'));
}

} // namespace fonts
