#include "EntityClassChooser.h"
#include "EntityClassTreePopulator.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "ithread.h"

#include <glibmm.h>

#include <wx/button.h>
#include <wx/panel.h>
#include <wx/splitter.h>

#include "gtkutil/TreeModel.h"
#include "string/string.h"
#include "registry/bind.h"
#include "eclass.h"

#include "debugging/ScopedDebugTimer.h"

#include <boost/make_shared.hpp>

namespace ui
{

namespace
{
    const char* const ECLASS_CHOOSER_TITLE = N_("Create entity");
    const std::string RKEY_SPLIT_POS = "user/ui/entityClassChooser/splitPos";
}

// Local class for loading entity class definitions in a separate thread
class EntityClassChooser::ThreadedEntityClassLoader
{
    // Column specification struct
    const EntityClassChooser::TreeColumns& _columns;

    // The tree store to populate. We must operate on our own tree store, since
    // updating the EntityClassChooser's tree store from a different thread
    // wouldn't be safe
    wxutil::TreeModel* _treeStore;

public:

    // Dispatcher to signal job finished
    Glib::Dispatcher signal_finished;

    // Construct and initialise variables
    ThreadedEntityClassLoader(const EntityClassChooser::TreeColumns& cols)
    : _columns(cols)
    { }

    // The worker function that will execute in the thread
    void run()
    {
        ScopedDebugTimer timer("ThreadedEntityClassLoader::run()");

        // Create new treestoree
		_treeStore = new wxutil::TreeModel(_columns);

        // Populate it with the list of entity classes by using a visitor class.
        EntityClassTreePopulator visitor(_treeStore, _columns);
        GlobalEntityClassManager().forEachEntityClass(visitor);

        // Insert the data, using the same walker class as Visitor
        visitor.forEachNode(visitor);

        signal_finished.emit();
    }

    // Return the populated treestore
	wxutil::TreeModel* getTreeStore()
    {
        return _treeStore;
    }
};

// Main constructor
EntityClassChooser::EntityClassChooser()
: wxutil::DialogBase(_(ECLASS_CHOOSER_TITLE)),
  _treeStore(NULL),
  _treeView(NULL),
  _eclassLoader(new ThreadedEntityClassLoader(_columns)),
  _selectedName("")
{
	loadNamedPanel(this, "EntityClassChooserMainPanel");

    // Connect button signals
    findNamedObject<wxButton>(this, "EntityClassChooserAddButton")->Connect(
        wxEVT_BUTTON, wxCommandEventHandler(EntityClassChooser::callbackOK), NULL, this);
	findNamedObject<wxButton>(this, "EntityClassChooserCancelButton")->Connect(
        wxEVT_BUTTON, wxCommandEventHandler(EntityClassChooser::callbackCancel), NULL, this);

    // Add model preview to right-hand-side of main container
	wxPanel* rightPanel = findNamedObject<wxPanel>(this, "EntityClassChooserRightPane");

	_modelPreview.reset(new wxutil::ModelPreview(rightPanel));

	rightPanel->GetSizer()->Add(_modelPreview->getWidget(), 1, wxEXPAND);

    // Listen for defs-reloaded signal (cannot bind directly to
    // ThreadedEntityClassLoader method because it is not sigc::trackable)
    GlobalEntityClassManager().defsReloadedSignal().connect(
        sigc::mem_fun(this, &EntityClassChooser::loadEntityClasses)
    );

    // Setup the tree view and invoke threaded loader to get the entity classes
    setupTreeView();
    loadEntityClasses();

    // Persist layout to registry
    // wxTODO registry::bindPropertyToKey(mainPaned->property_position(), RKEY_SPLIT_POS);

	Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(EntityClassChooser::callbackDeleteEvent), NULL, this);

	FitToScreen(0.7f, 0.6f);

	// Set the model preview height to something significantly smaller than the
    // window's height to allow shrinking
	_modelPreview->setSize(GetSize().GetWidth() * 0.4f,
		GetSize().GetHeight() * 0.2f);
}

void EntityClassChooser::getEntityClassesFromLoader()
{
    _treeStore = _eclassLoader->getTreeStore();
    setTreeViewModel();
}

// Display the singleton instance
std::string EntityClassChooser::chooseEntityClass(const std::string& preselectEclass)
{
	if (!preselectEclass.empty())
	{
		Instance().setSelectedEntityClass(preselectEclass);
	}

    if (Instance().ShowModal() == wxID_OK)
    {
        return Instance().getSelectedEntityClass();
    }
    else
    {
        return ""; // Empty selection on cancel
    }
}

EntityClassChooser& EntityClassChooser::Instance()
{
    EntityClassChooserPtr& instancePtr = InstancePtr();

    if (instancePtr == NULL)
    {
        // Not yet instantiated, do it now
        instancePtr.reset(new EntityClassChooser);

        // Register this instance with GlobalRadiant() at once
        GlobalRadiant().signal_radiantShutdown().connect(
            sigc::mem_fun(*instancePtr, &EntityClassChooser::onRadiantShutdown)
        );
    }

    return *instancePtr;
}

EntityClassChooserPtr& EntityClassChooser::InstancePtr()
{
    static EntityClassChooserPtr _instancePtr;
    return _instancePtr;
}

void EntityClassChooser::onRadiantShutdown()
{
    rMessage() << "EntityClassChooser shutting down." << std::endl;

    _modelPreview.reset();

    // Final step at shutdown, release the shared ptr
	Instance().SendDestroyEvent();
    InstancePtr().reset();
}

void EntityClassChooser::loadEntityClasses()
{
    g_assert(_eclassLoader);

    // Use a threaded job to load the classes
    _eclassLoader->signal_finished.connect(
        sigc::mem_fun(*this, &EntityClassChooser::getEntityClassesFromLoader)
    );
    GlobalRadiant().getThreadManager().execute(
        boost::bind(&ThreadedEntityClassLoader::run, _eclassLoader.get())
    );
}

void EntityClassChooser::setSelectedEntityClass(const std::string& eclass)
{
    // Select immediately if possible, otherwise remember class name for later
    // selection
    if (_treeStore != NULL)
    {
		wxDataViewItem item = _treeStore->FindString(eclass, _columns.name.getColumnIndex());

		if (item.IsOk())
		{
			_treeView->Select(item);
			_classToHighlight.clear();

			return;
		}
    }
     
	_classToHighlight = eclass;
}

const std::string& EntityClassChooser::getSelectedEntityClass() const
{
    return _selectedName;
}

void EntityClassChooser::callbackDeleteEvent(wxCloseEvent& ev)
{
	// greebo: Clear the selected name on hide, we don't want to create another entity when
    // the user clicks on the X in the upper right corner.
    _selectedName.clear();

	EndModal(wxID_CANCEL); // break main loop
	Hide();
}

int EntityClassChooser::ShowModal()
{
	// Update the member variables
    updateSelection();

    // Focus on the treeview
	_treeView->SetFocus();

	return DialogBase::ShowModal();
}

void EntityClassChooser::setTreeViewModel()
{
    // Ensure model is sorted before giving it to the tree view
	_treeStore->SortModelFoldersFirst(_columns.name, _columns.isFolder);

	_treeView->AssociateModel(_treeStore);
	_treeStore->DecRef();

    // Pre-select the given class if requested by setSelectedEntityClass()
    if (!_classToHighlight.empty())
    {
        g_assert(_treeStore);
        setSelectedEntityClass(_classToHighlight);
    }
}

void EntityClassChooser::setupTreeView()
{
    // Use the TreeModel's full string search function
	_treeStore = new wxutil::TreeModel(_columns);

	wxPanel* parent = findNamedObject<wxPanel>(this, "EntityClassChooserLeftPane");

    _treeView = new wxutil::TreeView(parent, wxDV_SINGLE);
	_treeView->AssociateModel(_treeStore);
	_treeStore->DecRef();

    // wxTODO view->set_search_equal_func(sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContains));

	_treeView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(EntityClassChooser::callbackSelectionChanged), NULL, this);

    // Single column with icon and name
	_treeView->AppendIconTextColumn(_("Classname"), _columns.name.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	parent->GetSizer()->Prepend(_treeView, 1, wxEXPAND | wxBOTTOM, 6);
}

// Update the usage information
void EntityClassChooser::updateUsageInfo(const std::string& eclass)
{
    // Lookup the IEntityClass instance
    IEntityClassPtr e = GlobalEntityClassManager().findOrInsert(eclass, true);

    // Set the usage panel to the IEntityClass' usage information string
    wxTextCtrl* usageText = findNamedObject<wxTextCtrl>(this, "EntityClassChooserUsageText");
    usageText->SetValue(eclass::getUsage(*e));
}

void EntityClassChooser::updateSelection()
{
	wxDataViewItem item = _treeView->GetSelection();

	if (item.IsOk())
    {
        wxutil::TreeModel::Row row(item, *_treeStore);

        if (!row[_columns.isFolder])
        {
            // Make the OK button active
            findNamedObject<wxButton>(this, "EntityClassChooserAddButton")->Enable(true);

            // Set the panel text with the usage information
            std::string selName = row[_columns.name];
            updateUsageInfo(selName);

            // Lookup the IEntityClass instance
            IEntityClassPtr eclass = GlobalEntityClassManager().findClass(selName);

            if (eclass != NULL)
            {
                _modelPreview->setModel(eclass->getAttribute("model").getValue());
                _modelPreview->setSkin(eclass->getAttribute("skin").getValue());
            }

            // Update the _selectionName field
            _selectedName = selName;

            return; // success
        }
    }

    // Nothing selected
    _modelPreview->setModel("");
    _modelPreview->setSkin("");

    findNamedObject<wxButton>(this, "EntityClassChooserAddButton")->Enable(false);
}

void EntityClassChooser::callbackCancel(wxCommandEvent& ev)
{
	_selectedName.clear();

	EndModal(wxID_CANCEL); // break main loop
	Hide();
}

void EntityClassChooser::callbackOK(wxCommandEvent& ev)
{
    EndModal(wxID_OK); // break main loop
	Hide();
}

void EntityClassChooser::callbackSelectionChanged(wxDataViewEvent& ev)
{
	updateSelection();
}


} // namespace ui
