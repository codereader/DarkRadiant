#pragma once

#include "imodule.h"
#include "irenderable.h"

#include <iostream>

class Material;
typedef boost::shared_ptr<Material> MaterialPtr;

namespace fonts
{

namespace q3font
{
	// Default values of Quake 3 sourcecode. Don't change!
	const std::size_t GLYPH_COUNT_PER_FONT = 256;

} // namespace

// Container-class for Glyphs (== single font characters).
class IGlyphInfo
{
public:
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
	std::string texture; // the texture name without extension, e.g. carleton_1_24

	// The shader this glyph is associated with
	// this is NULL until the font is actually used
	ShaderPtr shader;
};
typedef boost::shared_ptr<IGlyphInfo> IGlyphInfoPtr;

// Each D3 font has three resolutions
enum Resolution
{
	Resolution12,
	Resolution24,
	Resolution48,
	NumResolutions
};

inline std::ostream& operator<< (std::ostream& os, Resolution res)
{
    switch (res)
    {
        case Resolution12:
            os << "12";
            break;

        case Resolution24:
            os << "24";
            break;

        case Resolution48:
            os << "48";
            break;

        default:
            assert(false);
            os << "Unrecognised";
            break;
    }

    return os;
}

// Each font resolution has its own set of glyphs
class IGlyphSet
{
public:
	virtual ~IGlyphSet() {}

	// 12, 24, 48
	virtual Resolution getResolution() const = 0;

	// each set has 256 glyphs (q3font::GLYPH_COUNT_PER_FONT)
	virtual IGlyphInfoPtr getGlyph(std::size_t glyphIndex) const = 0;

	// Gets the scale for calculating the render font size
	virtual float getGlyphScale() const = 0;

	// Gets the maximum width of a glyph in this set
	virtual std::size_t getMaxGlyphWidth() const = 0;

	// Gets the maximum height of a glyph in this set
	virtual std::size_t getMaxGlyphHeight() const = 0;

	// Ensures that each glyph has a valid Shader
	virtual void realiseShaders() = 0;
};
typedef boost::shared_ptr<IGlyphSet> IGlyphSetPtr;

/**
 * Holds information about one specific font.
 * A font consists of three resolutions.
 */
class IFontInfo
{
public:
	virtual ~IFontInfo() {}

	// The name of the font, e.g. "carleton"
	virtual const std::string& getName() const = 0;

	// The language of this font
	virtual const std::string& getLanguage() const = 0;

	// Returns the glyphset for the specified resolution
	virtual IGlyphSetPtr getGlyphSet(Resolution resolution) = 0;
};
typedef boost::shared_ptr<IFontInfo> IFontInfoPtr;

/**
 * greebo: Use the FontManager to load a specific font. The returned FontInfo structure
 * contains all the necessary info about the glyphs, as found in the font's DAT file.
 */
class IFontManager :
	public RegisterableModule
{
public:
	// Returns the info structure of a specific font (current language),
	// returns NULL if no font info is available yet
	virtual IFontInfoPtr findFontInfo(const std::string& name) = 0;
};

}

const std::string MODULE_FONTMANAGER("FontManager");

inline fonts::IFontManager& GlobalFontManager()
{
	// Cache the reference locally
	static fonts::IFontManager& _fontManager(
		*boost::static_pointer_cast<fonts::IFontManager>(
			module::GlobalModuleRegistry().getModule(MODULE_FONTMANAGER)
		)
	);
	return _fontManager;
}
