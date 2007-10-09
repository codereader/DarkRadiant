#include "ImageLoaderManager.h"

#include "iimage.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <iostream>

namespace shaders {

	namespace {
		// Needed for boost::algorithm::split		
		typedef std::vector<std::string> StringParts;
	}

ImageLoaderList ImageLoaderManager::getLoaders(const std::string& moduleNames) {
	// The map that is to be filled
	ImageLoaderList list;
	
	StringParts parts;
	boost::algorithm::split(parts, moduleNames, boost::algorithm::is_any_of(" "));
	
	for (unsigned int i = 0; i < parts.size(); i++) {
		std::string fileExt = boost::to_upper_copy(parts[i]);
		
		// Acquire the module using the given fileExt
		ImageLoaderPtr loader = GlobalImageLoader(fileExt);
		
		if (loader != NULL) {
			list.push_back(loader);
		}
	}
	
	return list;
}

} // namespace shaders
