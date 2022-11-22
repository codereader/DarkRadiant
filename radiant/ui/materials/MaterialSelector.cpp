#include "MaterialSelector.h"

#include <vector>
#include <string>

#include "ishaders.h"

#include "texturelib.h"
#include "gamelib.h"
#include "string/predicate.h"

#include "../common/TexturePreviewCombo.h"
#include "wxutil/dataview/VFSTreePopulator.h"
#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"

namespace ui
{

/* CONSTANTS */

namespace
{
	constexpr const char* const TEXTURE_ICON = "icon_texture.png";
}

/**
 * Visitor class to retrieve material names and add them to folders.
 */
class ThreadedMaterialLoader final :
    public wxutil::ThreadedDeclarationTreePopulator
{
private:
    std::vector<std::string> _prefixes;

public:
    ThreadedMaterialLoader(const wxutil::DeclarationTreeView::Columns& columns, MaterialSelector::TextureFilter filter) :
        ThreadedDeclarationTreePopulator(decl::Type::Material, columns, TEXTURE_ICON)
    {
        switch (filter)
        {
        case MaterialSelector::TextureFilter::Lights:
            _prefixes = game::current::getLightTexturePrefixes();
            break;

        case MaterialSelector::TextureFilter::Regular:
            _prefixes = std::vector<std::string>{ GlobalMaterialManager().getTexturePrefix() };
            break;

        case MaterialSelector::TextureFilter::All:
            _prefixes = std::vector<std::string>();
            break;
        };
    }

    ~ThreadedMaterialLoader() override
    {
        EnsureStopped();
    }

protected:
    void PopulateModel(const wxutil::TreeModel::Ptr& model) override
    {
        wxutil::VFSTreePopulator populator(model);

        GlobalMaterialManager().foreachShaderName([&](const std::string& materialName)
        {
            if (_prefixes.empty()) // no filter?
            {
                AddMaterial(populator, materialName);
                return;
            }

            for (const std::string& prefix : _prefixes)
            {
                if (string::istarts_with(materialName, prefix))
                {
                    AddMaterial(populator, materialName);
                    break; // don't consider any further prefixes
                }
            }
        });
    }

    void AddMaterial(wxutil::VFSTreePopulator& populator, const std::string& materialName)
    {
        populator.addPath(materialName, [&](wxutil::TreeModel::Row& row,
            const std::string& path, const std::string& leafName, bool isFolder)
        {
            AssignValuesToRow(row, path, path, leafName, isFolder);
        });
    }
};

MaterialSelector::MaterialSelector(wxWindow* parent, const std::function<void()>& selectionChanged,
    TextureFilter textureFilter) :
    DeclarationSelector(parent, decl::Type::Material),
    _textureFilter(textureFilter),
	_selectionChanged(selectionChanged)
{
    AddPreviewToBottom(new TexturePreviewCombo(this));

    Populate();
}

void MaterialSelector::Populate()
{
    PopulateTreeView(std::make_shared<ThreadedMaterialLoader>(GetColumns(), _textureFilter));
}

MaterialPtr MaterialSelector::getSelectedShader()
{
	return GlobalMaterialManager().getMaterial(GetSelectedDeclName());
}

void MaterialSelector::onTreeViewSelectionChanged()
{
    if (_selectionChanged)
    {
        _selectionChanged();
    }
}

} // namespace
