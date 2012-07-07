#include "GlyphSet.h"

#include "idatastream.h"
#include "iarchive.h"
#include "ifilesystem.h"
#include "irender.h"

namespace fonts
{

// Construct a glyphset from Q3 info
GlyphSet::GlyphSet(const q3font::Q3FontInfo& q3info,
				   const std::string& fontname,
				   const std::string& language,
				   Resolution resolution_) :
	_glyphScale(q3info.glyphScale),
	_maxGlyphWidth(0),
	_maxGlyphHeight(0),
	resolution(resolution_)
{
	std::set<std::string> temp;

	// Construct all the glyphs
	for (std::size_t i = 0; i < q3font::GLYPH_COUNT_PER_FONT; ++i)
	{
		_glyphs[i] = GlyphInfoPtr(new GlyphInfo(q3info.glyphs[i]));

		// Check max glyph width and height
		if (_glyphs[i]->imageHeight > static_cast<int>(_maxGlyphHeight))
		{
			_maxGlyphHeight = _glyphs[i]->imageHeight;
		}

		if (_glyphs[i]->imageWidth > static_cast<int>(_maxGlyphWidth))
		{
			_maxGlyphWidth = _glyphs[i]->imageWidth;
		}

		// Memorise unique texture names
		temp.insert(_glyphs[i]->texture);
	}

	// Now construct the full texture paths, VFS-compatible
	for (std::set<std::string>::const_iterator i = temp.begin();
		 i != temp.end(); ++i)
	{
		_textures[*i] = "fonts/" + language + "/" + fontname + "/" + *i;
	}
}

GlyphSetPtr GlyphSet::createFromDatFile(const std::string& vfsPath,
										const std::string& fontname,
										const std::string& language,
										Resolution resolution)
{
	ArchiveFilePtr file = GlobalFileSystem().openFile(vfsPath);

	// Check file size
	if (file->size() != sizeof(q3font::Q3FontInfo))
	{
		rWarning() << "FontLoader: invalid file size of file "
			<< vfsPath << ", expected " << sizeof(q3font::Q3FontInfo)
			<< ", found " << file->size() << std::endl;
		return GlyphSetPtr();
	}

	// Allocate a buffer with the Quake3 info structure
	q3font::Q3FontInfoPtr buf(new q3font::Q3FontInfo);

	InputStream& stream = file->getInputStream();
	StreamBase::size_type bytesRead = stream.read(
		reinterpret_cast<StreamBase::byte_type*>(buf.get()),
		sizeof(q3font::Q3FontInfo)
	);

	// Construct a glyph set using the loaded info
	GlyphSetPtr glyphSet(new GlyphSet(*buf, fontname, language, resolution));

	rMessage() << "FontLoader: "  << vfsPath << " loaded successfully." << std::endl;

	return glyphSet;
}

void GlyphSet::realiseShaders()
{
	// For each glyph, acquire the appropriate shader
	for (std::size_t i = 0; i < q3font::GLYPH_COUNT_PER_FONT; ++i)
	{
		TexturePathMap::const_iterator found = _textures.find(_glyphs[i]->texture);
		assert(found != _textures.end());

		_glyphs[i]->shader = GlobalRenderSystem().capture(found->second);
	}
}

} // namespace fonts
