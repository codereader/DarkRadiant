#ifndef _GLYPHSET_H_
#define _GLYPHSET_H_

#include "GlyphInfo.h"
#include <boost/shared_ptr.hpp>
#include <map>

namespace fonts
{

class GlyphSet;
typedef boost::shared_ptr<GlyphSet> GlyphSetPtr;

// Each D3 font has three resolutions
enum Resolution
{
	Resolution12,
	Resolution24,
	Resolution48,
	NumResolutions
};

// Each font resolution has its own set of glyphs
class GlyphSet
{
private:
	// Private constructor, initialises this class using 
	// the Q3-style info structure (as read in from the .DAT file)
	GlyphSet(const q3font::Q3FontInfo& q3info, const std::string& fontname, 
		const std::string& language, Resolution resolution_);

public:
	// 12, 24, 48
	Resolution resolution;

	// each set has 256 glyphs
	GlyphInfoPtr glyphs[q3font::GLYPH_COUNT_PER_FONT];

	// Texture names => VFS path mapping
	// The texture name is the string as found in each Glyph
	// This map is used to acquire the shaders in realiseShaders()
	// File extension and dds/ prefix are omitted in the VFS paths
	typedef std::map<std::string, std::string> TexturePathMap;
	TexturePathMap textures;

	// Public named constructor
	static GlyphSetPtr createFromDatFile(const std::string& vfsPath, 
										 const std::string& fontname,
										 const std::string& language,
										 Resolution resolution);

	// Ensures that each glyph has a valid Shader
	void realiseShaders();
};
typedef boost::shared_ptr<GlyphSet> GlyphSetPtr;

} // namespace fonts

#endif /* _GLYPHSET_H_ */
