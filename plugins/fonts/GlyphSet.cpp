#include "GlyphSet.h"

#include "idatastream.h"
#include "iarchive.h"
#include "ifilesystem.h"

namespace fonts
{

// Construct a glyphset from Q3 info
GlyphSet::GlyphSet(const q3font::Q3FontInfo& q3info, 
				   const std::string& fontname, 
				   const std::string& language, 
				   Resolution resolution_) :
	resolution(resolution_)
{
	std::set<std::string> temp;

	// Construct all the glyphs
	for (std::size_t i = 0; i < q3font::GLYPH_COUNT_PER_FONT; ++i)
	{
		glyphs[i] = GlyphInfoPtr(new GlyphInfo(q3info.glyphs[i]));

		// Memorise unique texture names
		temp.insert(glyphs[i]->texture);
	}

	// Now construct the full texture paths, VFS-compatible
	for (std::set<std::string>::const_iterator i = temp.begin();
		 i != temp.end(); ++i)
	{
		textures.push_back("fonts/" + language + "/" + fontname + "/" + *i);
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
		globalWarningStream() << "FontLoader: invalid file size of file " 
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

	globalOutputStream() << "FontLoader: "  << vfsPath << " loaded successfully." << std::endl;
		
	return glyphSet;
}

} // namespace fonts
