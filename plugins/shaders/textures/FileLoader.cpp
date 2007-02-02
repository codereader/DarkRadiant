#include "FileLoader.h"

#include "ifilesystem.h"
#include "iarchive.h"
#include "archivelib.h"
#include "modulesystem/modulesmap.h"

template<typename Type>
class ModulesRef;
typedef ModulesRef<ImageLoader> ImageLoaderModulesRef;

namespace {

	class LoadImageVisitor :
		public ImageLoaderModules::Visitor
	{
		// The filename to load
		const std::string _name;
	
		// The reference to the pointer in the parent function
		Image*& _image;
	
	public:
		LoadImageVisitor(const std::string& name, Image*& image) :
			_name(name),
			_image(image)
		{}
	
		// Visit function called for each imageloader module.
		void visit(const char* moduleName, const ImageLoader& loader) {
			// Only do anything, if the image pointer is still NULL (i.e. the image load has not succeeded yet)
			if (_image == NULL) {
				// Construct a DirectoryArchiveFile out of the filename		
				DirectoryArchiveFile* file = new DirectoryArchiveFile(_name.c_str(), _name.c_str());
		
				if (!file->failed()) {
					// Try to invoke the imageloader with a reference to an ArchiveFile
					_image = loader.load(*file);
				}
				
				// Release the object from memory
				file->release();
			}
		}
	
	}; // class LoadImageVisitor

} // namespace

// The default case uses the GDK file loader (which covers a wide range of formats)
FileLoader::FileLoader(const std::string& filename) :
	_imageLoaders("GDK"),
	_filename(filename)
{}

// The default case uses the GDK file loader (which covers a wide range of formats)
FileLoader::FileLoader(const std::string& filename, const std::string& moduleNames) :
	_imageLoaders(moduleNames.c_str()),
	_filename(filename)
{}

Image* FileLoader::construct() {
	Image* image = NULL;

	// Instantiate a visitor class to cycle through the ImageLoader modules 
	LoadImageVisitor loadVisitor(_filename, image);
	
	// Cycle through all modules and tell them to visit the local class
	_imageLoaders.get().foreachModule(loadVisitor);
	
	return image;
}
