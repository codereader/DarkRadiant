#pragma once

#include "iimage.h"

/// ImageLoader implementation using wxImage to load an image from disk
class ImageLoaderWx : public ImageLoader
{
public:
    // ImageLoader implementation
	ImagePtr load(ArchiveFile& file) const;
	std::string getExtension() const;

	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name("ImageLoaderWX");
		return _name;
	}

	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies; // no dependencies
  		return _dependencies;
	}

	virtual void initialiseModule(const ApplicationContext& ctx);
};
