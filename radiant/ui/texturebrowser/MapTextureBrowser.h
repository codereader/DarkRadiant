#pragma once

#include <set>
#include "TextureThumbnailBrowser.h"

namespace ui
{
/**
 * Texture thumbnail browser implementation showing all materials
 * used in the current map ("in use").
 */
class MapTextureBrowser :
    public TextureThumbnailBrowser
{
private:
    // if true, the texture window will only display in-use shaders
    // if false, all the shaders in memory are displayed
    bool _hideUnused;
    bool _showFavouritesOnly;

    // Whether materials not starting with "textures/" should be visible
    bool _showOtherMaterials;

    // Cached set of material favourites
    std::set<std::string> _favourites;

public:
    MapTextureBrowser(wxWindow* parent);

protected:
    void populateTiles() override;

private:
    /**
     * Returns true if the given material is visible,
     * taking filter and showUnused into account.
     */
    bool materialIsVisible(const MaterialPtr& material);

    void onSettingsChanged();
    void observeRegistryKey(const std::string& key);

    void onActiveShadersChanged();
};

}
