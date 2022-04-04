#include "FontLoader.h"

#include "os/path.h"
#include "string/convert.h"

#include <regex>

#include "ifilesystem.h"
#include "itextstream.h"
#include "FontManager.h"
#include "xmlutil/MissingXMLNodeException.h"

namespace fonts
{

namespace
{
    const char* MISSING_BASEPATH_NODE =
        "Failed to find \"/game/filesystem/fonts/basepath\" node \
        in game descriptor";

    const char* MISSING_EXTENSION_NODE =
        "Failed to find \"/game/filesystem/fonts/extension\" node \
        in game descriptor";
}

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

std::string FontLoader::getFontPath()
{
    auto nlBasePath = GlobalGameManager().currentGame()->getLocalXPath("/filesystem/fonts/basepath");

    if (nlBasePath.empty())
    {
        throw xml::MissingXMLNodeException(MISSING_BASEPATH_NODE);
    }

    // Load the DAT files from the VFS
    return os::standardPathWithSlash(nlBasePath[0].getContent()) + _manager.getCurLanguage() + "/";
}

std::string FontLoader::getFontExtension()
{
    auto nlExt = GlobalGameManager().currentGame()->getLocalXPath("/filesystem/fonts/extension");

    if (nlExt.empty())
    {
        throw xml::MissingXMLNodeException(MISSING_EXTENSION_NODE);
    }

    return nlExt[0].getContent();
}

void FontLoader::loadFonts()
{
    GlobalFileSystem().forEachFile(getFontPath(), getFontExtension(), 
        std::bind(&FontLoader::loadFont, this, std::placeholders::_1), 2);

    rMessage() << _manager.getNumFonts() << " fonts registered." << std::endl;
}

} // namespace fonts
