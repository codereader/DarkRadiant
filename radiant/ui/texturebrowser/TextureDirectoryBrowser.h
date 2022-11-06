#pragma once

#include "ishaders.h"
#include "TextureThumbnailBrowser.h"
#include "os/path.h"

namespace ui
{
/**
 * Special TextureThumbnailBrowser displaying all textures matching
 * a given directory prefix, e.g. "textures/common/".
 */
class TextureDirectoryBrowser :
    public TextureThumbnailBrowser
{
private:
    std::string _texturePath;

public:
    TextureDirectoryBrowser(wxWindow* parent, const std::string& texturePath) :
        TextureThumbnailBrowser(parent),
        _texturePath(os::standardPathWithSlash(texturePath))
    {}

protected:
    void populateTiles() override
    {
        GlobalMaterialManager().foreachShaderName([&](const std::string& name)
        {
            // Check if this material is matching the prefix
            if (!string::istarts_with(name, _texturePath)) return;

            // Ignore any subdirectories, just direct leafs
            auto partAfterPrefix = name.substr(_texturePath.length());
            if (partAfterPrefix.find('/') != std::string::npos) return;

            createTileForMaterial(GlobalMaterialManager().getMaterial(name));
        });
    }
};

}
