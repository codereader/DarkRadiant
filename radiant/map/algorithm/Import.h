#pragma once

#include <istream>
#include <string>

namespace map
{

class MapFormat;
typedef std::shared_ptr<MapFormat> MapFormatPtr;

namespace algorithm
{

/**
 * Returns a map format capable of loading the data in the given stream,
 * matching the the given extension. Since more than one map format might
 * be able to load the map data (e.g. .pfb vs .map), the extension gives this
 * method a hint about which one to prefer.
 * Passing an empty extension will consider all available formats.
 * The stream needs to support the seek method.
 * After this call, the stream is guaranteed to be rewound to the beginning
  */
MapFormatPtr determineMapFormat(std::istream& stream, const std::string& type);

/**
 * Returns a map format capable of loading the data in the given stream
 * The stream needs to support the seek method
 * After this call, the stream is guaranteed to be rewound to the beginning
  */
MapFormatPtr determineMapFormat(std::istream& stream);

}

}
