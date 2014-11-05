#include "EntityChooser.h"

#include "i18n.h"
#include "inode.h"
#include "ientity.h"
#include "imainframe.h"
#include "iscenegraph.h"

#include <wx/panel.h>
#include <wx/sizer.h>

namespace ui
{

EntityChooser::EntityChooser() :
	DialogBase(_("Select Entity")),
	_entityStore(new wxutil::TreeModel(_listColumns, true)),
	_entityView(NULL)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

	_entityView = wxutil::TreeView::Create(this, wxDV_NO_HEADER);

	vbox->Add(_entityView, 1, wxEXPAND | wxBOTTOM, 12);
	vbox->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT);

	FitToScreen(0.3f, 0.6f);

	// Use the TreeModel's full string search function
	_entityView->AddSearchColumn(_listColumns.name);

	// Head Name column
	_entityView->AppendTextColumn("#", _listColumns.name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	populateEntityList();
}

std::string EntityChooser::getSelectedEntity() const
{
	// Prepare to check for a selection
	wxDataViewItem item = _entityView->GetSelection();

	if (item.IsOk())
	{
		wxutil::TreeModel::Row row(item, *_entityStore);
		return row[_listColumns.name];
	}
	else
	{
		return "";
	}
}

void EntityChooser::setSelectedEntity(const std::string& name)
{
	wxDataViewItem item = _entityStore->FindString(name, _listColumns.name);

	if (item.IsOk())
	{
		_entityView->Select(item);
		_selectedEntityName = name;
	}
}

std::string EntityChooser::ChooseEntity(const std::string& preSelectedEntity)
{
	std::string returnValue;

	EntityChooser* chooser = new EntityChooser;

	chooser->setSelectedEntity(preSelectedEntity);

	if (chooser->ShowModal() == wxID_OK)
	{
		returnValue = chooser->getSelectedEntity();
	}

	chooser->Destroy();

	return returnValue;
}

void EntityChooser::populateEntityList()
{
	struct EntityFinder:
		public scene::NodeVisitor
	{
        // List store to add to
        wxutil::TreeModel::Ptr _store;

		EntityChooserColumns& _columns;

        // Constructor
		EntityFinder(wxutil::TreeModel::Ptr store,
					 EntityChooserColumns& columns) :
			_store(store),
			_columns(columns)
		{}

        // Visit function
        bool pre(const scene::INodePtr& node)
		{
			// Check for an entity
            Entity* entity = Node_getEntity(node);

            if (entity != NULL)
			{
				// Get the entity name
                std::string entName = entity->getKeyValue("name");

				// Append the name to the list store
				wxutil::TreeModel::Row row = _store->AddItem();
				row[_columns.name] = entName;
				row.SendItemAdded();
            }

            return false; // don't traverse deeper, we're traversing root children
        }
    } finder(_entityStore, _listColumns);

	GlobalSceneGraph().root()->traverseChildren(finder);

	_entityStore->SortModelByColumn(_listColumns.name);

	_entityView->AssociateModel(_entityStore.get());
}

} // namespace
