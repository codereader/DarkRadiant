#include "MaterialsList.h"

#include "i18n.h"
#include "irender.h"

#include <iostream>

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
}

MaterialsList::MaterialsList(wxWindow* parent, const RenderSystemPtr& renderSystem) :
	wxutil::TreeView(parent, wxutil::TreeModel::Ptr(), wxDV_SINGLE),
	_store(new wxutil::TreeModel(COLUMNS(), true)),
	_renderSystem(renderSystem)
{
    assert(_renderSystem);

	AssociateModel(_store.get());

	EnableAutoColumnWidthFix(false); // we don't need this

	// View columns
	AppendTextColumn(_("Material"), COLUMNS().material.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);
	AppendToggleColumn(_("Visible"), COLUMNS().visible.getColumnIndex(), 
		wxDATAVIEW_CELL_ACTIVATABLE, wxCOL_WIDTH_AUTOSIZE);

	Connect(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, 
		wxDataViewEventHandler(MaterialsList::onShaderToggled), NULL, this);
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

void MaterialsList::clear()
{
    _store->Clear();
}

void MaterialsList::addMaterial(const std::string& name)
{
	wxutil::TreeModel::Row row = _store->AddItem();

    ShaderPtr shader = _renderSystem->capture(name);

    row[COLUMNS().material] = name;
    row[COLUMNS().visible] = shader && shader->isVisible();

	row.SendItemAdded();
}

}
