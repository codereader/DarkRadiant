#include "EClassTree.h"

#include "ieclass.h"
#include "ientity.h"
#include "iselection.h"
#include "i18n.h"

#include "EClassTreeBuilder.h"

#include "wxutil/dataview/ResourceTreeViewToolbar.h"

#include <wx/sizer.h>
#include <wx/splitter.h>
#include "wxutil/Bitmap.h"

namespace ui
{

namespace
{
	constexpr const char* const ECLASSTREE_TITLE = N_("Entity Class Tree");
}

EClassTree::EClassTree() :
	DialogBase(_(ECLASSTREE_TITLE)),
	_eclassView(nullptr),
	_propertyStore(nullptr),
	_propertyView(nullptr)
{
	// Construct the window's widgets
	populateWindow();

    _eclassView->Populate(std::make_shared<EClassTreeBuilder>(_eclassColumns));
}

void EClassTree::onTreeViewPopulationFinished(wxutil::ResourceTreeView::PopulationFinishedEvent& ev)
{
	std::string className;

	// Do we have anything selected
	if (GlobalSelectionSystem().countSelected() > 0)
	{
		// Get the last selected node and check if it's an entity
		auto lastSelected = GlobalSelectionSystem().ultimateSelected();

		if (const auto* entity = Node_getEntity(lastSelected); entity)
		{
			// There is an entity selected, extract the classname
            className = entity->getKeyValue("classname");
		}
	}

	if (!className.empty())
	{
        _eclassView->SetSelectedDeclName(className);
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

	auto* splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
    splitter->SetMinimumPaneSize(10); // disallow unsplitting

	auto eclassPane = createEClassTreeView(splitter);
	createPropertyTreeView(splitter);

	splitter->SplitVertically(eclassPane, _propertyView);

	GetSizer()->Add(splitter, 1, wxEXPAND | wxALL, 12);
	GetSizer()->Add(CreateStdDialogButtonSizer(wxCLOSE), 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);

	SetAffirmativeId(wxID_CLOSE);

	// Set the default size of the window
	Layout();
	FitToScreen(0.8f, 0.8f);

	splitter->SetSashPosition(static_cast<int>(GetSize().GetWidth() * 0.25f));
}

wxWindow* EClassTree::createEClassTreeView(wxWindow* parent)
{
    auto panel = new wxPanel(parent);

	_eclassView = new wxutil::DeclarationTreeView(panel, decl::Type::EntityDef, _eclassColumns);

	// Use the TreeModel's full string search function
    _eclassView->AddSearchColumn(_eclassColumns.leafName);
    _eclassView->EnableSetFavouritesRecursively(false);

	// Tree selection
	_eclassView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &EClassTree::onSelectionChanged, this);
    _eclassView->Bind(wxutil::EV_TREEVIEW_POPULATION_FINISHED, &EClassTree::onTreeViewPopulationFinished, this);

	// Single column with icon and name
	_eclassView->AppendIconTextColumn(_("Classname"), _eclassColumns.iconAndName.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);

    auto toolbar = new wxutil::ResourceTreeViewToolbar(panel, _eclassView);

    panel->SetSizer(new wxBoxSizer(wxVERTICAL));
    panel->GetSizer()->Add(toolbar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    panel->GetSizer()->Add(_eclassView, 1, wxEXPAND);

    return panel;
}

void EClassTree::createPropertyTreeView(wxWindow* parent)
{
	// Initialise the instance TreeStore
	_propertyStore = new wxutil::TreeModel(_propertyColumns, true);

    // Create the TreeView widget and link it to the model
	_propertyView = wxutil::TreeView::CreateWithModel(parent, _propertyStore.get());

    // Create the Property column
	_propertyView->AppendTextColumn(_("Property"), _propertyColumns.name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	_propertyView->AppendTextColumn(_("Value"), _propertyColumns.value.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
}

void EClassTree::addToListStore(const EntityClassAttribute& attr, bool inherited)
{
    // Append the details to the treestore
    wxutil::TreeModel::Row row = _propertyStore->AddItem();

	wxDataViewItemAttr colour;
	colour.SetColour(inherited ? wxColor(127, 127, 127) : wxColor(0, 0, 0));

    row[_propertyColumns.name] = attr.getName();
	row[_propertyColumns.name].setAttr(colour);

    row[_propertyColumns.value] = attr.getValue();
	row[_propertyColumns.value].setAttr(colour);

    row[_propertyColumns.inherited] = inherited ? "1" : "0";

	row.SendItemAdded();
}

void EClassTree::updatePropertyView(const std::string& eclassName)
{
	// Clear the existing list
	_propertyStore->Clear();

	auto eclass = GlobalEntityClassManager().findClass(eclassName);

    if (!eclass) return;

    eclass->forEachAttribute([&](const EntityClassAttribute& attr, bool inherited)
    {
        addToListStore(attr, inherited);
    }, true);
}

// Static command target
void EClassTree::ShowDialog(const cmd::ArgumentList& args)
{
	// Construct a new instance, this enters the main loop
	auto* tree = new EClassTree();

	tree->ShowModal();
	tree->Destroy();
}

void EClassTree::handleSelectionChange()
{
	// Prepare to check for a selection
	auto item = _eclassView->GetSelection();

	// Add button is enabled if there is a selection and it is not a folder.
	if (item.IsOk())
	{
		_propertyView->Enable(true);

		// Set the panel text with the usage information
		wxutil::TreeModel::Row row(item, *_eclassView->GetTreeModel());

		updatePropertyView(row[_eclassColumns.declName].getString().ToStdString());
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
