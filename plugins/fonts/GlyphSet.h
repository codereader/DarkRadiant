#ifndef _GLYPHSET_H_
#define _GLYPHSET_H_

#include "GlyphInfo.h"
#include <boost/shared_ptr.hpp>
#include <map>

namespace fonts
{

class GlyphSet;
typedef boost::shared_ptr<GlyphSet> GlyphSetPtr;

// Each font resolution has its own set of glyphs
class GlyphSet :
	public IGlyphSet
{
private:
	// Texture names => VFS path mapping
	// The texture name is the string as found in each Glyph
	// This map is used to acquire the shaders in realiseShaders()
	// File extension and dds/ prefix are omitted in the VFS paths
	typedef std::map<std::string, std::string> TexturePathMap;
	TexturePathMap _textures;

	float _glyphScale;

	// Maximum size of a glyph in this set
	std::size_t _maxGlyphWidth;
	std::size_t _maxGlyphHeight;

	// each set has 256 glyphs
	GlyphInfoPtr _glyphs[q3font::GLYPH_COUNT_PER_FONT];

	// Private constructor, initialises this class using
	// the Q3-style info structure (as read in from the .DAT file)
	GlyphSet(const q3font::Q3FontInfo& q3info, const std::string& fontname,
		const std::string& language, Resolution resolution_);
public:
	// 12, 24, 48
	Resolution resolution;

	// Public named constructor
	static GlyphSetPtr createFromDatFile(const std::string& vfsPath,
										 const std::string& fontname,
										 const std::string& language,
										 Resolution resolution);

	// IGlyphSet implementation
	Resolution getResolution() const
	{
		return resolution;
	}

	IGlyphInfoPtr getGlyph(std::size_t glyphIndex) const
	{
		assert(glyphIndex < q3font::GLYPH_COUNT_PER_FONT);
		return _glyphs[glyphIndex];
	}

	float getGlyphScale() const
	{
		return _glyphScale;
	}

	std::size_t getMaxGlyphWidth() const
	{
		return _maxGlyphWidth;
	}

	std::size_t getMaxGlyphHeight() const
	{
		return _maxGlyphHeight;
	}

	// Ensures that each glyph has a valid Shader
	void realiseShaders();
};
typedef boost::shared_ptr<GlyphSet> GlyphSetPtr;

} // namespace fonts

#endif /* _GLYPHSET_H_ */
