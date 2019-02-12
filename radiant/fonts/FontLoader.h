#include "ifonts.h"

#include "ifilesystem.h"

namespace fonts
{

class FontManager;

class FontLoader
{
private:
	// The base path for the shaders (e.g. "materials/")
	std::string _basePath;

	// The manager for registering the fonts
	FontManager& _manager;

public:
	// Constructor. Set the base path of the search.
	FontLoader(const std::string& path, FontManager& manager) :
		_basePath(path),
		_manager(manager)
	{}

	void operator()(const vfs::FileInfo& fileInfo);
};

} // namespace fonts
