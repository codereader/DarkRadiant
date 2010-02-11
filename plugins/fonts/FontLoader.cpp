#include "FontLoader.h"

#include "os/path.h"
#include <boost/regex.hpp>

#include "FontManager.h"

namespace fonts 
{

void FontLoader::operator() (const std::string& filename)
{
	// Construct the full VFS path
	std::string fullPath = os::standardPath(_basePath + filename);

	boost::regex expr("^/?(.*)/.*_(\\d{2})\\.dat$", boost::regex::icase);
	boost::smatch matches;
	
	if (boost::regex_match(filename, matches, expr))
	{
		// Get the font name and resolution from the match
		std::string fontname = matches[1];
		std::string resolution = matches[2];

		// Create the font (if not done yet), acquire the info structure
		FontInfoPtr font = _manager.findOrCreateFontInfo(fontname);

		// TODO: load the info
	}
}

} // namespace fonts 
