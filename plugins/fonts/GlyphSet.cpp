#include "GlyphSet.h"

#include "idatastream.h"
#include "iarchive.h"
#include "ifilesystem.h"

namespace fonts
{

namespace q3font
{
	// Default values of Quake 3 sourcecode. Don't change!
	struct glyphInfo_t
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
		glyphInfo_t glyphs[q3font::GLYPH_COUNT_PER_FONT];
		float glyphScale;
		char name[q3font::FONT_NAME_LENGTH];
	};
	typedef boost::shared_ptr<Q3FontInfo> Q3FontInfoPtr;
}

GlyphSetPtr GlyphSet::createFromDatFile(const std::string& vfsPath)
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

	// TODO: Now translate the info into our glyph set
	GlyphSetPtr glyphSet(new GlyphSet);

	globalOutputStream() << "FontLoader: "  << vfsPath << " loaded successfully." << std::endl;
		
	return glyphSet;
}

} // namespace fonts
