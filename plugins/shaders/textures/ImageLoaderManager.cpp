#include "ImageLoaderManager.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

#include <iostream>

namespace shaders {

	namespace {
		// Needed for boost::algorithm::split		
		typedef std::vector<std::string> StringParts;
	}

ImageLoaderManager::ImageLoaderManager() :
	_imageLoaders("*")
{}

ImageLoaderList ImageLoaderManager::getLoaders(const std::string& moduleNames) {
	// Get the reference to the map of allocated imageloader modules 
	ModulesMap<ImageLoader>& modulesMap = _imageLoaders.get();
	
	// The map that is to be filled
	ImageLoaderList list;
	
	StringParts parts;
	boost::algorithm::split(parts, moduleNames, boost::algorithm::is_any_of(" "));
	
	for (unsigned int i = 0; i < parts.size(); i++) {
		ImageLoader* loader = modulesMap.find(parts[i]);
		if (loader != NULL) {
			list.push_back(loader);
		}
	}
	
	return list;
}

ImageLoaderManager& ImageLoaderManager::Instance() {
	static ImageLoaderManager _instance;
	return _instance;
}
	
} // namespace shaders
