#include "FontLoader.h"

#include "os/path.h"
#include "string/convert.h"

#include <boost/regex.hpp>

#include "itextstream.h"
#include "FontManager.h"

namespace fonts
{

void FontLoader::visit(const std::string& filename)
{
	// Construct the full VFS path
	std::string fullPath = os::standardPath(_basePath + filename);

	boost::regex expr("^/?(.*)/.*_(\\d{2})\\.dat$", boost::regex::icase);
	boost::smatch matches;

	if (boost::regex_match(filename, matches, expr))
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
			FontInfoPtr font = _manager.findOrCreateFontInfo(fontname);

			// Load the DAT file and create the glyph info
			font->glyphSets[resolution] = GlyphSet::createFromDatFile(
				fullPath, fontname, _manager.getCurLanguage(), resolution
			);
		}
		else
		{
			rWarning() << "FontLoader: ignoring DAT: " << filename << std::endl;
		}
	}
}

} // namespace fonts
