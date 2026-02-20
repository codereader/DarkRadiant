#include "MaterialThumbnailBrowser.h"

#include "ishaders.h"
#include "gamelib.h"
#include "string/predicate.h"
#include "string/case_conv.h"
#include "string/split.h"

namespace ui
{

MaterialThumbnailBrowser::MaterialThumbnailBrowser(wxWindow* parent, TextureFilter filter) :
    TextureThumbnailBrowser(parent, false),
    _textureFilter(filter)
{
    switch (_textureFilter)
    {
    case TextureFilter::Lights:
        _prefixes = game::current::getLightTexturePrefixes();
        break;
    case TextureFilter::Regular:
        _prefixes = std::vector<std::string>{ GlobalMaterialManager().getTexturePrefix() };
        break;
    case TextureFilter::All:
        _prefixes = std::vector<std::string>();
        break;
    }
}

sigc::signal<void()>& MaterialThumbnailBrowser::signal_selectionChanged()
{
    return _signalSelectionChanged;
}

sigc::signal<void()>& MaterialThumbnailBrowser::signal_itemActivated()
{
    return _signalItemActivated;
}

void MaterialThumbnailBrowser::setExternalFilterText(const std::string& filter)
{
    if (_externalFilter != filter)
    {
        _externalFilter = filter;
        queueUpdate();
    }
}

void MaterialThumbnailBrowser::populateTiles()
{
    GlobalMaterialManager().foreachShaderName([&](const std::string& materialName)
    {
        if (!_prefixes.empty())
        {
            bool matchesPrefix = false;
            for (const std::string& prefix : _prefixes)
            {
                if (string::istarts_with(materialName, prefix))
                {
                    matchesPrefix = true;
                    break;
                }
            }
            if (!matchesPrefix) return;
        }

        if (materialIsFiltered(materialName))
        {
            return;
        }

        if (!_externalFilter.empty())
        {
            std::string lowerName = string::to_lower_copy(materialName);
            std::vector<std::string> filters;
            string::split(filters, string::to_lower_copy(_externalFilter), " ");

            for (const auto& f : filters)
            {
                if (lowerName.find(f) == std::string::npos)
                {
                    return;
                }
            }
        }

        auto material = GlobalMaterialManager().getMaterial(materialName);
        if (material && material->getEditorImage())
        {
            createTileForMaterial(material);
        }
    });
}

void MaterialThumbnailBrowser::handleMaterialSelection(const MaterialPtr& material)
{
    _signalSelectionChanged.emit();
}

void MaterialThumbnailBrowser::handleMaterialActivated(const MaterialPtr& material)
{
    _signalItemActivated.emit();
}

} // namespace ui
