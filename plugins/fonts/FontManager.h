#ifndef FontManager_h__
#define FontManager_h__

#include "ifonts.h"
#include "iimage.h"

#include "string/string.h"
#include <map>
#include <boost/shared_array.hpp>

#include "FontInfo.h"

namespace fonts
{

//Quake 3 font related typedefs:
/*namespace q3font
{

namespace
{
	//Default values of Quake 3 sourcecode. Don't change!
	const int ShaderNameLength = 32;
	const int GlyphCountPerFont = 256;
	const int FontNameLength = 64;
}

typedef int		qhandle_t;

typedef struct {
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
	qhandle_t glyph;  // handle to the shader with the glyph
	char shaderName[ShaderNameLength];
} glyphInfo_t;

typedef struct {
	glyphInfo_t glyphs [GlyphCountPerFont];
	float glyphScale;
	char name[FontNameLength];
} fontInfo_t;

} //namespace q3font

typedef boost::shared_ptr<q3font::fontInfo_t> Q3FontInfoPtr;
typedef boost::shared_array<ImagePtr> ImagePtrArray;

// Container-class for Glyphs.
class GlyphInfo
{
public:
	int _height;       // number of scan lines
	int _top;          // top of glyph in buffer
	int _bottom;       // bottom of glyph in buffer
	int _pitch;        // width for copying
	int _xSkip;        // x adjustment
	int _imageWidth;   // width of actual image
	int _imageHeight;  // height of actual image
	float _s;          // x offset in image where glyph starts
	float _t;          // y offset in image where glyph starts
	float _s2;
	float _t2;
	int _textureIndex;

	GlyphInfo() {}

	// Contructor with all attributes 
	GlyphInfo(int height, int top, int bottom, int pitch, int xSkip,
		int imageWidth, int imageHeight, float s, float t, float s2, float t2,
		std::string shaderName) :
		_height(height), _top(top), _bottom(bottom),
		_pitch(pitch), _xSkip(xSkip), _imageWidth(imageWidth), _imageHeight(imageHeight),
		_s(s), _t(t), _s2(s2), _t2(t2),
		_textureIndex(strToInt(shaderName.substr(shaderName.length()-8,1))) {} //dirty
};
typedef boost::shared_ptr<GlyphInfo> GlyphInfoPtr;*/

/* Container-class for Fonts.*/
/*class FontInfo
{
public:
	GlyphInfoPtr _glyphs[q3font::GlyphCountPerFont];
	float _glyphScale;
	std::string _fontName;
	std::string _fontResolution;
	int _maxTextureIndex;
	std::string _texturePath;

	FontInfo() {};
	FontInfo(float glyphScale, std::string name);
};
typedef boost::shared_ptr<FontInfo> FontInfoPtr;*/

class FontManager :
	public IFontManager
{
private:
	typedef std::map<std::string, FontInfoPtr> FontMap;
	FontMap _fonts;

public:
	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();

	// Returns the info structure of a specific font (current language),
	// returns NULL if no font info is available yet
	FontInfoPtr findFontInfo(const std::string& name);

	// Returns the info structure of a specific font (current language),
	// always returns non-NULL, non-existent fonts are created on the fly
	FontInfoPtr findOrCreateFontInfo(const std::string& name);

private:
	void reloadFonts();
};
typedef boost::shared_ptr<FontManager> FontManagerPtr;

} // namespace fonts

#endif // FontManager_h__
