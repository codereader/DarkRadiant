#include "EntityList.h"

#include "ieventmanager.h"
#include "imainframe.h"
#include "iuimanager.h"

#include "registry/Widgets.h"
#include "entitylib.h"
#include "iselectable.h"
#include "icamera.h"
#include "i18n.h"

#include "wxutil/TreeView.h"

#include <wx/sizer.h>
#include <wx/checkbox.h>

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Entity List");
	const std::string RKEY_ROOT = "user/ui/entityList/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";

	const std::string RKEY_ENTITYLIST_FOCUS_SELECTION = RKEY_ROOT + "focusSelection";
	const std::string RKEY_ENTITYLIST_VISIBLE_ONLY = RKEY_ROOT + "visibleNodesOnly";
}

EntityList::EntityList() :
	wxutil::TransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow(), true),
	_callbackActive(false)
{
	// Create all the widgets and pack them into the window
	populateWindow();

	// Connect the window position tracker
	InitialiseWindowPosition(300, 800, RKEY_WINDOW_STATE);
}
    
EntityList::~EntityList()
{
    // In OSX we might receive callbacks during shutdown, so disable any events
    if (_treeView != nullptr)
    {
        _treeView->Disconnect(wxEVT_DATAVIEW_SELECTION_CHANGED,
                       wxDataViewEventHandler(EntityList::onSelection), NULL, this);
        _treeView->Disconnect(wxEVT_DATAVIEW_ITEM_EXPANDED,
                       wxDataViewEventHandler(EntityList::onRowExpand), NULL, this);
    }
}

void EntityList::populateWindow()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

	// Configure the treeview
	_treeView = wxutil::TreeView::CreateWithModel(
        this, _treeModel.getModel(), 
#if defined(__linux__)
		wxDV_MULTIPLE
#else
		wxDV_NO_HEADER | wxDV_MULTIPLE
#endif
    );

	// Single column with icon and name
	_treeView->AppendTextColumn(_("Name"), _treeModel.getColumns().name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    // Enable type-ahead searches
    _treeView->AddSearchColumn(_treeModel.getColumns().name);

	_treeView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(EntityList::onSelection), NULL, this);

	_treeView->Connect(wxEVT_DATAVIEW_ITEM_EXPANDED, 
		wxDataViewEventHandler(EntityList::onRowExpand), NULL, this);

	// Update the toggle item status according to the registry

	_focusSelected = new wxCheckBox(this, wxID_ANY, _("Focus camera on selected entity"));
	_visibleOnly = new wxCheckBox(this, wxID_ANY, _("List visible nodes only"));

	registry::bindWidget(_focusSelected, RKEY_ENTITYLIST_FOCUS_SELECTION);
    registry::bindWidget(_visibleOnly, RKEY_ENTITYLIST_VISIBLE_ONLY);

	vbox->Add(_treeView, 1, wxEXPAND | wxBOTTOM, 6);
	vbox->Add(_focusSelected, 0, wxBOTTOM, 6);
	vbox->Add(_visibleOnly, 0);

	_treeModel.setConsiderVisibleNodesOnly(_visibleOnly->GetValue());

	// Connect the toggle buttons' "toggled" signal
	_visibleOnly->Connect(wxEVT_CHECKBOX, 
		wxCommandEventHandler(EntityList::onVisibleOnlyToggle), NULL, this);
}

void EntityList::update()
{
	// Disable callbacks and traverse the treemodel
	_callbackActive = true;

	// Traverse the entire tree, updating the selection
	_treeModel.updateSelectionStatus(std::bind(&EntityList::onTreeViewSelection, this,
		std::placeholders::_1, std::placeholders::_2));

	_callbackActive = false;
}

void EntityList::refreshTreeModel()
{
    // Refresh the whole tree
    _selection.clear();

    _treeModel.refresh();

    // If the model changed, associate the newly created model with our
    // treeview
    if (_treeModel.getModel().get() != _treeView->GetModel())
    {
        _treeView->AssociateModel(_treeModel.getModel().get());
    }

    expandRootNode();
}

// Gets notified upon selection change
void EntityList::selectionChanged(const scene::INodePtr& node, bool isComponent)
{
	if (_callbackActive || !IsShownOnScreen() || isComponent)
	{
		// Don't update if not shown or already updating, also ignore components
		return;
	}

	_callbackActive = true;

	_treeModel.updateSelectionStatus(node, std::bind(&EntityList::onTreeViewSelection, this,
		std::placeholders::_1, std::placeholders::_2));

	_callbackActive = false;
}

void EntityList::filtersChanged()
{
    // Only react to filter changes if we display visible nodes only otherwise
    // we don't care
	if (_visibleOnly->GetValue())
	{
		// When filters are changed possibly any node could have changed 
        // its visibility, so refresh the whole tree
        refreshTreeModel();
	}
}

// Pre-hide callback
void EntityList::_preHide()
{
	TransientWindow::_preHide();

	_treeModel.disconnectFromSceneGraph();

	// Disconnect from the filters-changed signal
	_filtersChangedConnection.disconnect();

	// De-register self from the SelectionSystem
	GlobalSelectionSystem().removeObserver(this);
    
    // Unselect everything when hiding the dialog
    _callbackActive = true;
    
    _treeView->UnselectAll();
    
    _callbackActive = false;
}

// Pre-show callback
void EntityList::_preShow()
{
	TransientWindow::_preShow();

	// Observe the scenegraph
	_treeModel.connectToSceneGraph();

	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);

	// Get notified when filters are changing
	_filtersChangedConnection = GlobalFilterSystem().filtersChangedSignal().connect(
        sigc::mem_fun(Instance(), &EntityList::filtersChanged)
    );

	_callbackActive = true;

	// Repopulate the model before showing the dialog
    refreshTreeModel();

	_callbackActive = false;

	// Update the widgets
	update();

	expandRootNode();
}

void EntityList::toggle(const cmd::ArgumentList& args)
{
	Instance().ToggleVisibility();
}

void EntityList::onRadiantShutdown()
{
	if (IsShownOnScreen())
	{
		Hide();
	}

	// Destroy the window (after it has been disconnected from the Eventmanager)
	SendDestroyEvent();
	InstancePtr().reset();
}

EntityListPtr& EntityList::InstancePtr()
{
	static EntityListPtr _instancePtr;
	return _instancePtr;
}

EntityList& EntityList::Instance()
{
	if (InstancePtr() == NULL)
	{
		// Not yet instantiated, do it now
		InstancePtr() = EntityListPtr(new EntityList);

		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().signal_radiantShutdown().connect(
            sigc::mem_fun(*InstancePtr(), &EntityList::onRadiantShutdown)
        );
	}

	return *InstancePtr();
}

void EntityList::onRowExpand(wxDataViewEvent& ev)
{
	if (_callbackActive) return; // avoid loops

	// greebo: This is a possible optimisation point. Don't update the entire tree,
	// but only the expanded subtree.
	update();
}

void EntityList::onVisibleOnlyToggle(wxCommandEvent& ev)
{
    _treeModel.setConsiderVisibleNodesOnly(_visibleOnly->GetValue());

	// Update the whole tree
	refreshTreeModel();
}

void EntityList::expandRootNode()
{
	GraphTreeNodePtr rootNode = _treeModel.find(GlobalSceneGraph().root());

	if (!_treeView->IsExpanded(rootNode->getIter()))
	{
		_treeView->Expand(rootNode->getIter());
	}
}

void EntityList::onTreeViewSelection(const wxDataViewItem& item, bool selected)
{
	if (selected)
	{
		// Select the row in the TreeView
		_treeView->Select(item);

		// Remember this item
		_selection.insert(item);

		// Scroll to the row
		_treeView->EnsureVisible(item);
	}
	else
	{
		_treeView->Unselect(item);

		_selection.erase(item);
	} 
}

void EntityList::onSelection(wxDataViewEvent& ev)
{
	if (_callbackActive) return; // avoid loops

	wxutil::TreeView* view = static_cast<wxutil::TreeView*>(ev.GetEventObject());

	wxDataViewItemArray newSelection;
	view->GetSelections(newSelection);

	std::sort(newSelection.begin(), newSelection.end(), DataViewItemLess());

	std::vector<wxDataViewItem> diff(newSelection.size() + _selection.size());

	// Calculate the difference between these two sets
	std::vector<wxDataViewItem>::iterator end = std::set_symmetric_difference(
		newSelection.begin(), newSelection.end(), _selection.begin(), _selection.end(), diff.begin());

	for (std::vector<wxDataViewItem>::iterator i = diff.begin(); i != end; ++i)
	{
		// Load the instance pointer from the columns
		wxutil::TreeModel::Row row(*i, *_treeModel.getModel());
		scene::INode* node = static_cast<scene::INode*>(row[_treeModel.getColumns().node].getPointer());

		ISelectable* selectable = dynamic_cast<ISelectable*>(node);

		if (selectable != NULL)
		{
			// We've found a selectable instance

			// Disable update to avoid loopbacks
			_callbackActive = true;

			// Select the instance
			bool isSelected = view->IsSelected(*i);
			selectable->setSelected(isSelected);

			if (isSelected && _focusSelected->GetValue())
			{
				const AABB& aabb = node->worldAABB();
				Vector3 origin(aabb.origin);

				// Move the camera a bit off the AABB origin
				origin += Vector3(-50, 0, 50);

				// Rotate the camera a bit towards the "ground"
				Vector3 angles(0, 0, 0);
				angles[CAMERA_PITCH] = -30;

				GlobalCameraView().focusCamera(origin, angles);
			}

			// Now reactivate the callbacks
			_callbackActive = false;
		}
	}

	_selection.clear();
	_selection.insert(newSelection.begin(), newSelection.end());
}

} // namespace ui
