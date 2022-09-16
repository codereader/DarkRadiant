#include "ShaderSelector.h"

#include <vector>
#include <string>

#include "ishaders.h"

#include <wx/sizer.h>

#include "texturelib.h"
#include "gamelib.h"
#include "string/predicate.h"

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
    ThreadedMaterialLoader(const wxutil::DeclarationTreeView::Columns& columns, ShaderSelector::TextureFilter filter) :
        ThreadedDeclarationTreePopulator(decl::Type::Material, columns, TEXTURE_ICON)
    {
        if (filter == ShaderSelector::TextureFilter::Lights)
        {
            _prefixes = game::current::getLightTexturePrefixes();
        }
        else
        {
            _prefixes = std::vector<std::string>{ GlobalMaterialManager().getTexturePrefix() };
        }
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
            for (const std::string& prefix : _prefixes)
            {
                if (string::istarts_with(materialName, prefix))
                {
                    populator.addPath(materialName, [&](wxutil::TreeModel::Row& row,
                        const std::string& path, const std::string& leafName, bool isFolder)
                    {
                        AssignValuesToRow(row, path, path, leafName, isFolder);
                    });
                    break; // don't consider any further prefixes
                }
            }
        });
    }
};

ShaderSelector::ShaderSelector(wxWindow* parent, const std::function<void()>& selectionChanged,
    TextureFilter textureFilter) :
    DeclarationSelector(parent, decl::Type::Material, Columns()),
    _textureFilter(textureFilter),
	_selectionChanged(selectionChanged)
{
    // Pack in info panel
	createPreview();

    PopulateTreeView(std::make_shared<ThreadedMaterialLoader>(Columns(), _textureFilter));
}

const wxutil::DeclarationTreeView::Columns& ShaderSelector::Columns()
{
    static wxutil::DeclarationTreeView::Columns _shaderColumns;
    return _shaderColumns;
}

void ShaderSelector::createPreview()
{
    _previewCombo = new TexturePreviewCombo(this);
    GetSizer()->Add(_previewCombo, 0, wxEXPAND | wxTOP, 3);
}

MaterialPtr ShaderSelector::getSelectedShader()
{
	return GlobalMaterialManager().getMaterial(GetSelectedDeclName());
}

void ShaderSelector::onTreeViewSelectionChanged()
{
    _previewCombo->SetTexture(GetSelectedDeclName());

    if (_selectionChanged)
    {
        _selectionChanged();
    }
}

} // namespace
