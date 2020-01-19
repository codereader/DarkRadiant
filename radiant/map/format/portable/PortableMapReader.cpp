#include "PortableMapReader.h"

#include <regex>
#include "PortableMapFormat.h"

namespace map
{

namespace format
{

PortableMapReader::PortableMapReader(IMapImportFilter& importFilter) :
	_importFilter(importFilter)
{}

void PortableMapReader::readFromStream(std::istream& stream)
{

}

bool PortableMapReader::CanLoad(std::istream& stream)
{
	// Instead of instantiating an XML parser and buffering the whole
	// file into memory, read in some lines and look for 
	// certain signature parts.
	// This will fail if the XML is oddly formatted with e.g. line-breaks
	std::string buffer(512, '\0');

	for (int i = 0; i < 25; ++i)
	{
		stream.getline(buffer.data(), buffer.length());

		// Check if the format="portable" string is occurring somewhere
		std::regex pattern(R"(<map[^>]+format=\"portable\")");

		if (std::regex_search(buffer, pattern))
		{
			// Try to extract the version number
			std::regex versionPattern(R"(<map[^>]+version=\"(\d+)\")");
			std::smatch results;

			if (std::regex_search(buffer, results, versionPattern) &&
				string::convert<std::size_t>(results[1].str()) <= PortableMapFormat::VERSION)
			{
				return true;
			}
		}
	}

	return false;
}

}

}
