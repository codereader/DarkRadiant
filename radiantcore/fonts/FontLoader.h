#pragma once

#include "ifonts.h"

#include "ifilesystem.h"
#include "ThreadedDefLoader.h"

namespace fonts
{

class FontManager;

class FontLoader :
    public util::ThreadedDefLoader<void>
{
private:
	// The manager for registering the fonts
	FontManager& _manager;

public:
	FontLoader(FontManager& manager) :
        util::ThreadedDefLoader<void>(std::bind(&FontLoader::loadFonts, this)),
		_manager(manager)
	{}

private:
    void loadFonts();
    void loadFont(const vfs::FileInfo& fileInfo);

    std::string getFontPath();
    std::string getFontExtension();
};

} // namespace fonts
