#include "DefaultConstructor.h"

#include "ifilesystem.h"
#include "iarchive.h"
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
				// Construct the full name of the image to load, including the prefix (e.g. "dds/")
				// and the file extension.
				std::string fullName = loader.getPrefix() + _name + "." + loader.getExtension();
				
				// Try to open the file (will fail if the extension does not fit)
				ArchiveFile* file = GlobalFileSystem().openFile(fullName.c_str());
				
				// Has the file been loaded?
				if (file != NULL) {
					// Try to invoke the imageloader with a reference to the ArchiveFile
					_image = loader.load(*file);
					
					// Release the loaded file
					file->release();
				}
			}
		}
	
	}; // class LoadImageVisitor

} // namespace

// The default case uses the file extensions specified in the .game file
DefaultConstructor::DefaultConstructor(const std::string& filename) :
	_imageLoaders(GlobalRadiant().getRequiredGameDescriptionKeyValue("texturetypes")),
	_filename(filename)
{}

Image* DefaultConstructor::construct() {
	Image* image = NULL;

	// Instantiate a visitor class to cycle through the ImageLoader modules 
	LoadImageVisitor loadVisitor(_filename, image);
	
	// Cycle through all modules and tell them to visit the local class
	_imageLoaders.get().foreachModule(loadVisitor);
	
	return image;
}
