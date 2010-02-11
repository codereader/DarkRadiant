#include "ifonts.h"

namespace fonts
{

class FontLoader
{
private:
	// The base path for the shaders (e.g. "materials/")
	std::string _basePath;

public:
	// Constructor. Set the base path of the search.
	FontLoader(const std::string& path) : 
		_basePath(path)
	{}

	// Required functor typedef
	typedef const std::string& first_argument_type;

	void operator() (const std::string& fileName);
};

} // namespace fonts
