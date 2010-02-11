#include "ifonts.h"

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

	// Required functor typedef
	typedef const std::string& first_argument_type;

	void operator() (const std::string& fileName);
};

} // namespace fonts
