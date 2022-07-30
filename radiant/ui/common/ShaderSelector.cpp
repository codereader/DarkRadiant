#include "ShaderSelector.h"

#include <vector>
#include <string>

#include "i18n.h"
#include "ishaders.h"

#include <wx/sizer.h>

#include "texturelib.h"
#include "string/split.h"
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
    const ShaderSelector::PrefixList& _prefixes;

public:
    ThreadedMaterialLoader(const wxutil::DeclarationTreeView::Columns& columns, const ShaderSelector::PrefixList& prefixes) :
        ThreadedDeclarationTreePopulator(decl::Type::Material, columns, TEXTURE_ICON),
        _prefixes(prefixes)
    {}

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
                if (string::istarts_with(materialName, prefix + "/"))
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
    const std::string& prefixes) :
	wxPanel(parent, wxID_ANY),
	_treeView(nullptr),
	_selectionChanged(selectionChanged)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	// Split the given comma-separated list into the vector
	string::split(_prefixes, prefixes, ",");

	// Pack in TreeView and info panel
	createTreeView();
	createPreview();
}

std::string ShaderSelector::getSelection()
{
    return _treeView->GetSelectedDeclName();
}

void ShaderSelector::setSelection(const std::string& sel)
{
    _treeView->SetSelectedDeclName(sel);
}

void ShaderSelector::createTreeView()
{
    _treeView = new wxutil::DeclarationTreeView(this, decl::Type::Material, 
        _shaderTreeColumns, wxDV_NO_HEADER | wxDV_SINGLE);

    // Single visible column, containing the directory/shader name and the icon
    _treeView->AppendIconTextColumn(_("Value"), _shaderTreeColumns.iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    // Use the TreeModel's full string search function
    _treeView->AddSearchColumn(_shaderTreeColumns.leafName);

    // Get selection and connect the changed callback
    _treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ShaderSelector::_onSelChange, this);

    GetSizer()->Add(_treeView, 1, wxEXPAND);

    _treeView->Populate(std::make_shared<ThreadedMaterialLoader>(_shaderTreeColumns, _prefixes));
}

void ShaderSelector::createPreview()
{
    _previewCombo = new TexturePreviewCombo(this);
    GetSizer()->Add(_previewCombo, 0, wxEXPAND | wxTOP, 3);
}

MaterialPtr ShaderSelector::getSelectedShader()
{
	return GlobalMaterialManager().getMaterial(getSelection());
}

void ShaderSelector::_onSelChange(wxDataViewEvent& ev)
{
    _previewCombo->SetTexture(getSelection());

    if (_selectionChanged)
    {
        _selectionChanged();
    }
}

} // namespace
