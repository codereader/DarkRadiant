#include "Import.h"

#include "imapformat.h"

namespace map
{

namespace algorithm
{

MapFormatPtr determineMapFormat(std::istream& stream, const std::string& type)
{
	// Get all registered map formats matching the extension
	auto availableFormats = type.empty() ?
		GlobalMapFormatManager().getAllMapFormats() :
		GlobalMapFormatManager().getMapFormatList(type);

	MapFormatPtr format;

	for (const auto& candidate : availableFormats)
	{
		// Rewind the stream before passing it to the format for testing
		// Map format valid, rewind the stream
		stream.seekg(0, std::ios_base::beg);

		if (candidate->canLoad(stream))
		{
			format = candidate;
			break;
		}
	}

	// Rewind the stream when we're done
	stream.seekg(0, std::ios_base::beg);

	return format;
}

MapFormatPtr determineMapFormat(std::istream& stream)
{
	return determineMapFormat(stream, std::string());
}

}

}
