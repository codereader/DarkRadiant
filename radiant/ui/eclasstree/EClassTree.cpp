#include "EClassTree.h"

#include "ieclass.h"
#include "ientity.h"
#include "imainframe.h"
#include "iselection.h"

#include "EClassTreeBuilder.h"
#include "i18n.h"
#include "iuimanager.h"

#include <functional>

#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/artprov.h>

namespace ui 
{

namespace
{
	const char* const ECLASSTREE_TITLE = N_("Entity Class Tree");
}

EClassTree::EClassTree() :
	DialogBase(_(ECLASSTREE_TITLE)),
	_eclassStore(NULL),
	_eclassView(NULL),
	_propertyStore(NULL),
	_propertyView(NULL)
{
	// Create a new tree store for the entityclasses
	_eclassStore = new wxutil::TreeModel(_eclassColumns);

	// Construct the window's widgets
	populateWindow();

	wxutil::TreeModel::Row row = _eclassStore->AddItem();

	wxIcon icon;
	icon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "cmenu_add_entity.png"));
	row[_eclassColumns.name] = wxVariant(wxDataViewIconText(_("Loading, please wait..."), icon));

	row.SendItemAdded();

	// Construct an eclass visitor and traverse the entity classes
	_treeBuilder.reset(new EClassTreeBuilder(_eclassColumns, this));

	Connect(wxutil::EV_TREEMODEL_POPULATION_FINISHED, 
		TreeModelPopulationFinishedHandler(EClassTree::onTreeStorePopulationFinished), NULL, this);

	_treeBuilder->populate();
}

void EClassTree::onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev)
{
	_eclassStore = ev.GetTreeModel();
	wxDataViewItem preselectItem;

	// Do we have anything selected
	if (GlobalSelectionSystem().countSelected() > 0)
	{
		// Get the last selected node and check if it's an entity
		scene::INodePtr lastSelected = GlobalSelectionSystem().ultimateSelected();

		Entity* entity = Node_getEntity(lastSelected);

		if (entity != NULL)
		{
			// There is an entity selected, extract the classname
			std::string classname = entity->getKeyValue("classname");

			// Find and select the classname
			preselectItem = _eclassStore->FindString(classname, _eclassColumns.name);
		}
	}

	_eclassView->AssociateModel(_eclassStore.get());

	if (preselectItem.IsOk())
	{
		_eclassView->Select(preselectItem);
		_eclassView->EnsureVisible(preselectItem);
		handleSelectionChange();
	}
	else
	{
		// Expand the top level items if nothing should be preselected
		_eclassView->ExpandTopLevelItems();
	}
}

void EClassTree::populateWindow()
{
	// Create the overall vbox
	SetSizer(new wxBoxSizer(wxVERTICAL));
	
	wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, 
		wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
    splitter->SetMinimumPaneSize(10); // disallow unsplitting

	createEClassTreeView(splitter);
	createPropertyTreeView(splitter);

	splitter->SplitVertically(_eclassView, _propertyView);

	GetSizer()->Add(splitter, 1, wxEXPAND | wxALL, 12);
	GetSizer()->Add(CreateStdDialogButtonSizer(wxCLOSE), 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);

	SetAffirmativeId(wxID_CLOSE);

	// Set the default size of the window
	Layout();
	FitToScreen(0.8f, 0.8f);

	splitter->SetSashPosition(static_cast<int>(GetSize().GetWidth() * 0.25f));
}

void EClassTree::createEClassTreeView(wxWindow* parent)
{
	_eclassView = wxutil::TreeView::CreateWithModel(parent, _eclassStore);

	// Use the TreeModel's full string search function
	_eclassView->AddSearchColumn(_eclassColumns.name);

	// Tree selection
	_eclassView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(EClassTree::onSelectionChanged), NULL, this);

	// Single column with icon and name
	_eclassView->AppendIconTextColumn(_("Classname"), _eclassColumns.name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);
}

void EClassTree::createPropertyTreeView(wxWindow* parent)
{
	// Initialise the instance TreeStore
	_propertyStore = new wxutil::TreeModel(_propertyColumns, true);

    // Create the TreeView widget and link it to the model
	_propertyView = wxutil::TreeView::CreateWithModel(parent, _propertyStore);

    // Create the Property column
	_propertyView->AppendTextColumn(_("Property"), _propertyColumns.name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	_propertyView->AppendTextColumn(_("Value"), _propertyColumns.value.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
}

void EClassTree::addToListStore(const EntityClassAttribute& attr)
{
    // Append the details to the treestore
    wxutil::TreeModel::Row row = _propertyStore->AddItem();

	wxDataViewItemAttr colour;
	colour.SetColour(attr.inherited ? wxColor(127, 127, 127) : wxColor(0, 0, 0));

    row[_propertyColumns.name] = attr.getName();
	row[_propertyColumns.name] = colour;

    row[_propertyColumns.value] = attr.getValue();
	row[_propertyColumns.value] = colour;

    row[_propertyColumns.inherited] = attr.inherited ? "1" : "0";

	row.SendItemAdded();
}

void EClassTree::updatePropertyView(const std::string& eclassName)
{
	// Clear the existing list
	_propertyStore->Clear();

	IEntityClassPtr eclass = GlobalEntityClassManager().findClass(eclassName);

	if (eclass == NULL)
    {
		return;
	}

	eclass->forEachClassAttribute(
        std::bind(&EClassTree::addToListStore, this, std::placeholders::_1), true
    );
}

// Static command target
void EClassTree::ShowDialog(const cmd::ArgumentList& args)
{
	// Construct a new instance, this enters the main loop
	EClassTree* tree = new EClassTree;

	tree->ShowModal();
	tree->Destroy();
}

void EClassTree::handleSelectionChange()
{
	// Prepare to check for a selection
	wxDataViewItem item = _eclassView->GetSelection();

	// Add button is enabled if there is a selection and it is not a folder.
	if (item.IsOk())
	{
		_propertyView->Enable(true);

		// Set the panel text with the usage information
		wxutil::TreeModel::Row row(item, *_eclassStore);

		wxDataViewIconText value = row[_eclassColumns.name];

		updatePropertyView(value.GetText().ToStdString());
	}
	else
	{
		_propertyView->Enable(false);
	}
}

void EClassTree::onSelectionChanged(wxDataViewEvent& ev)
{
	handleSelectionChange();
}

} // namespace ui
