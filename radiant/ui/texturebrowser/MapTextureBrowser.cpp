#include "MapTextureBrowser.h"

#include "ifavourites.h"

#include "shaderlib.h"
#include "string/case_conv.h"
#include "string/split.h"

#include "TextureBrowserManager.h"

namespace ui
{

MapTextureBrowser::MapTextureBrowser(wxWindow* parent) :
    TextureThumbnailBrowser(parent)
{
    observeRegistryKey(RKEY_TEXTURES_HIDE_UNUSED);
    observeRegistryKey(RKEY_TEXTURES_SHOW_FAVOURITES_ONLY);
    observeRegistryKey(RKEY_TEXTURES_SHOW_OTHER_MATERIALS);

    onSettingsChanged();

    GlobalMaterialManager().signal_activeShadersChanged().connect(
        sigc::mem_fun(this, &MapTextureBrowser::onActiveShadersChanged));
}

void MapTextureBrowser::observeRegistryKey(const std::string& key)
{
    GlobalRegistry().signalForKey(key).connect(
        sigc::mem_fun(this, &MapTextureBrowser::onSettingsChanged)
    );
}

void MapTextureBrowser::onSettingsChanged()
{
    _hideUnused = registry::getValue<bool>(RKEY_TEXTURES_HIDE_UNUSED);
    _showFavouritesOnly = registry::getValue<bool>(RKEY_TEXTURES_SHOW_FAVOURITES_ONLY);
    _showOtherMaterials = registry::getValue<bool>(RKEY_TEXTURES_SHOW_OTHER_MATERIALS);
}

void MapTextureBrowser::populateTiles()
{
    // Update the favourites
    _favourites = GlobalFavouritesManager().getFavourites(decl::getTypeName(decl::Type::Material));

    GlobalMaterialManager().foreachMaterial([&](const MaterialPtr& mat)
    {
        if (!materialIsVisible(mat))
        {
            return;
        }

        createTileForMaterial(mat);
    });
}

bool MapTextureBrowser::materialIsVisible(const MaterialPtr& material)
{
    if (!material) return false;

    auto materialName = material->getName();

    if (!_showOtherMaterials && !string::istarts_with(material->getName(), GlobalTexturePrefix_get()))
    {
        return false;
    }

    if (_hideUnused && !material->IsInUse())
    {
        return false;
    }

    if (_showFavouritesOnly && _favourites.count(materialName) == 0)
    {
        return false;
    }

    return !materialIsFiltered(materialName);
}

void MapTextureBrowser::onActiveShadersChanged()
{
    queueUpdate();
}

}
