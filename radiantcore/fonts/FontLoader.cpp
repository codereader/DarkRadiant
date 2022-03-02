#include "FontLoader.h"

#include "os/path.h"
#include "string/convert.h"

#include <regex>

#include "itextstream.h"
#include "FontManager.h"

namespace fonts
{

void FontLoader::loadFont(const vfs::FileInfo& fileInfo)
{
	// Construct the full VFS path
	std::string fullPath = fileInfo.fullPath();

	std::regex expr("^/?(.*)/.*_(\\d{2})\\.dat$", std::regex::icase);
	std::smatch matches;

	if (std::regex_match(fileInfo.name, matches, expr))
	{
		// Get the font name and resolution from the match
		std::string fontname = matches[1];
		std::string resolutionStr = matches[2];

		int r = string::convert<int>(resolutionStr);

		Resolution resolution = NumResolutions;

		switch (r)
		{
		case 12:
			resolution = Resolution12;
			break;
		case 24:
			resolution = Resolution24;
			break;
		case 48:
			resolution = Resolution48;
			break;
		};

		if (resolution != NumResolutions)
		{
			// Create the font (if not done yet), acquire the info structure
			auto font = _manager.findOrCreateFontInfo(fontname);

			// Load the DAT file and create the glyph info
			font->glyphSets[resolution] = GlyphSet::createFromDatFile(
				fullPath, fontname, _manager.getCurLanguage(), resolution
			);
		}
		else
		{
			rWarning() << "FontLoader: ignoring DAT: " << fileInfo.name << std::endl;
		}
	}
}

void FontLoader::loadFonts()
{
    loadFiles(std::bind(&FontLoader::loadFont, this, std::placeholders::_1));

    rMessage() << _manager.getNumFonts() << " fonts registered." << std::endl;
}

} // namespace fonts
