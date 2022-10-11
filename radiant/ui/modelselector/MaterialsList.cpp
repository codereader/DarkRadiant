#include "MaterialsList.h"

#include "i18n.h"
#include "irender.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "wxutil/sourceview/DeclarationSourceView.h"

namespace ui
{

namespace
{
    // Columns for the list
    struct Columns : 
		public wxutil::TreeModel::ColumnRecord
    {
        wxutil::TreeModel::Column material;
        wxutil::TreeModel::Column visible;

        Columns() :
			material(add(wxutil::TreeModel::Column::String)),
			visible(add(wxutil::TreeModel::Column::Boolean))
		{}
    };

    const Columns& COLUMNS()
    {
        static const Columns _instance;
        return _instance;
    }

    constexpr const char* const SHOW_MATERIAL_DEF_TEXT = N_("Show Material Definition");
    constexpr const char* const SHOW_MATERIAL_DEF_ICON = "icon_script.png";
}

MaterialsList::MaterialsList(wxWindow* parent, const RenderSystemPtr& renderSystem) :
	wxutil::TreeView(parent, nullptr, wxDV_SINGLE),
	_store(new wxutil::TreeModel(COLUMNS(), true)),
	_renderSystem(renderSystem),
    _contextMenu(new wxutil::PopupMenu)
{
    assert(_renderSystem);

	AssociateModel(_store.get());

	EnableAutoColumnWidthFix(false); // we don't need this

	// View columns
	AppendTextColumn(_("Material"), COLUMNS().material.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);
	AppendToggleColumn(_("Visible"), COLUMNS().visible.getColumnIndex(), 
		wxDATAVIEW_CELL_ACTIVATABLE, wxCOL_WIDTH_AUTOSIZE);

	Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &MaterialsList::onShaderToggled, this);
	Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &MaterialsList::onContextMenu, this);

    // Block all double-click events originating from this view, to not confuse
    // parent widgets with double-click events
    Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, [](wxDataViewEvent& ev) { ev.StopPropagation(); });

    // Construct the context menu
    _contextMenu->addItem(
        new wxutil::IconTextMenuItem(_(SHOW_MATERIAL_DEF_TEXT), SHOW_MATERIAL_DEF_ICON),
        std::bind(&MaterialsList::onShowShaderDefinition, this),
        [this]() { return !getSelectedMaterial().empty(); }
    );
}

void MaterialsList::clear()
{
    _store->Clear();
}

void MaterialsList::updateFromModel(const model::IModel& model)
{
    clear();

    // Add the list of active materials
    const auto& matList(model.getActiveMaterials());

    for (const auto& material : matList)
    {
        addMaterial(material);
    }
}

void MaterialsList::onShaderToggled(wxDataViewEvent& ev)
{
	wxDataViewItem item = ev.GetItem();
	if (!item.IsOk()) return;

    // Toggle the model data
	wxutil::TreeModel::Row row(item, *_store);
    
    // Hide or show the respective shader
    assert(_renderSystem);
    ShaderPtr shader = _renderSystem->capture(
        row[COLUMNS().material]
    );
    shader->setVisible(row[COLUMNS().visible].getBool());

    _visibilityChangedSignal.emit();
}

void MaterialsList::onContextMenu(wxDataViewEvent& ev)
{
    _contextMenu->show(this);
}

void MaterialsList::onShowShaderDefinition()
{
    // Construct a definition view and pass the material name
    auto view = new wxutil::DeclarationSourceView(this);
    
    view->setDeclaration(decl::Type::Material, getSelectedMaterial());
    view->ShowModal();
    view->Destroy();
}

void MaterialsList::addMaterial(const std::string& name)
{
	wxutil::TreeModel::Row row = _store->AddItem();

    ShaderPtr shader = _renderSystem->capture(name);

    row[COLUMNS().material] = name;
    row[COLUMNS().visible] = shader && shader->isVisible();

	row.SendItemAdded();
}

std::string MaterialsList::getSelectedMaterial()
{
    wxDataViewItem item = GetSelection();
    if (!item.IsOk()) return "";

    wxutil::TreeModel::Row row(item, *_store);

    return row[COLUMNS().material];
}

}
