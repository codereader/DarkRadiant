#pragma once

#include "ifonts.h"

#include "ifilesystem.h"
#include "parser/ThreadedDeclParser.h"

namespace fonts
{

class FontManager;

class FontLoader :
    public parser::ThreadedDeclParser<void>
{
private:
	// The manager for registering the fonts
	FontManager& _manager;

    std::size_t _numFonts;

public:
	// Constructor. Set the base path of the search.
	FontLoader(const std::string& path, const std::string& extension, FontManager& manager) :
        ThreadedDeclParser(path, extension, 2, std::bind(&FontLoader::loadFonts, this)),
		_manager(manager),
        _numFonts(0)
	{}

private:
    void loadFonts();
    void loadFont(const vfs::FileInfo& fileInfo);
};

} // namespace fonts
