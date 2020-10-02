#include "EntityClassChooser.h"
#include "TreeModel.h"
#include "VFSTreePopulator.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "gamelib.h"

#include <wx/thread.h>

#include <wx/button.h>
#include <wx/panel.h>
#include <wx/splitter.h>

#include "string/string.h"
#include "eclass.h"

#include "debugging/ScopedDebugTimer.h"

namespace wxutil
{

namespace
{
    const char* const ECLASS_CHOOSER_TITLE = N_("Create entity");
    const std::string RKEY_SPLIT_POS = "user/ui/entityClassChooser/splitPos";

    const char* const FOLDER_ICON = "folder16.png";
    const char* const ENTITY_ICON = "cmenu_add_entity.png";

    // Registry XPath to lookup key that specifies the display folder
    const char* const FOLDER_KEY_PATH = "/entityChooser/displayFolderKey";
}

/*
 * EntityClassVisitor which populates a Gtk::TreeStore with entity classnames
 * taking account of display folders and mod names.
 */
class EntityClassTreePopulator:
    public wxutil::VFSTreePopulator,
    public EntityClassVisitor
{
    // TreeStore to populate
    wxutil::TreeModel::Ptr _store;

    // Column definition
    const wxutil::EntityClassChooser::TreeColumns& _columns;

    // Key that specifies the display folder
    std::string _folderKey;

    wxIcon _folderIcon;
    wxIcon _entityIcon;

public:

    // Constructor
    EntityClassTreePopulator(wxutil::TreeModel::Ptr store,
                             const EntityClassChooser::TreeColumns& columns)
    : wxutil::VFSTreePopulator(store),
      _store(store),
      _columns(columns),
      _folderKey(game::current::getValue<std::string>(FOLDER_KEY_PATH))
    {
        _folderIcon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + FOLDER_ICON));
        _entityIcon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + ENTITY_ICON));
    }

    // EntityClassVisitor implementation
    void visit(const IEntityClassPtr& eclass)
    {
        std::string folderPath = eclass->getAttribute(_folderKey).getValue();

        if (!folderPath.empty())
        {
            folderPath = "/" + folderPath;
        }

        // Create the folder to put this EntityClass in, depending on the value
        // of the DISPLAY_FOLDER_KEY.
        addPath(
            eclass->getModName() + folderPath + "/" + eclass->getName(),
            [this](wxutil::TreeModel::Row& row, const std::string& path,
                   bool isFolder)
            {
                // Get the display name by stripping off everything before the
                // last slash
                row[_columns.name] = wxVariant(
                    wxDataViewIconText(path.substr(path.rfind("/") + 1),
                                       !isFolder ? _entityIcon : _folderIcon)
                );
                row[_columns.isFolder] = isFolder;
                row.SendItemAdded();
            }
        );
    }
};

// Local class for loading entity class definitions in a separate thread
class EntityClassChooser::ThreadedEntityClassLoader :
    public wxThread
{
    // Column specification struct
    const EntityClassChooser::TreeColumns& _columns;

    // The tree store to populate. We must operate on our own tree store, since
    // updating the EntityClassChooser's tree store from a different thread
    // wouldn't be safe
    wxutil::TreeModel::Ptr _treeStore;

    // The class to be notified on finish
    wxEvtHandler* _finishedHandler;

public:

    // Construct and initialise variables
    ThreadedEntityClassLoader(const EntityClassChooser::TreeColumns& cols,
                              wxEvtHandler* finishedHandler) :
        wxThread(wxTHREAD_JOINABLE),
        _columns(cols),
        _finishedHandler(finishedHandler)
    {}

    ~ThreadedEntityClassLoader()
    {
        if (IsRunning())
        {
            Delete();
        }
    }

    // The worker function that will execute in the thread
    ExitCode Entry()
    {
        ScopedDebugTimer timer("ThreadedEntityClassLoader::run()");

        // Create new treestoree
        _treeStore = new wxutil::TreeModel(_columns);

        // Populate it with the list of entity classes by using a visitor class.
        EntityClassTreePopulator visitor(_treeStore, _columns);
        GlobalEntityClassManager().forEachEntityClass(visitor);

        if (TestDestroy()) return static_cast<ExitCode>(0);

        // Ensure model is sorted before giving it to the tree view
        _treeStore->SortModelFoldersFirst(_columns.name, _columns.isFolder);

        if (!TestDestroy())
        {
            wxQueueEvent(_finishedHandler, new wxutil::TreeModel::PopulationFinishedEvent(_treeStore));
        }

        return static_cast<ExitCode>(0);
    }
};

// Main constructor
EntityClassChooser::EntityClassChooser()
: wxutil::DialogBase(_(ECLASS_CHOOSER_TITLE)),
  _treeStore(nullptr),
  _treeView(nullptr),
  _selectedName("")
{
    // Connect the finish callback to load the treestore
    Bind(wxutil::EV_TREEMODEL_POPULATION_FINISHED, &EntityClassChooser::onTreeStorePopulationFinished, this);

    loadNamedPanel(this, "EntityClassChooserMainPanel");

    // Connect button signals
    findNamedObject<wxButton>(this, "EntityClassChooserAddButton")->Bind(
        wxEVT_BUTTON, &EntityClassChooser::onOK, this);
    findNamedObject<wxButton>(this, "EntityClassChooserAddButton")->SetBitmap(
        wxArtProvider::GetBitmap(wxART_PLUS));

    findNamedObject<wxButton>(this, "EntityClassChooserCancelButton")->Bind(
        wxEVT_BUTTON, &EntityClassChooser::onCancel, this);

    // Add model preview to right-hand-side of main container
    wxPanel* rightPanel = findNamedObject<wxPanel>(this, "EntityClassChooserRightPane");

    _modelPreview.reset(new wxutil::ModelPreview(rightPanel));

    rightPanel->GetSizer()->Add(_modelPreview->getWidget(), 1, wxEXPAND);

    // Listen for defs-reloaded signal (cannot bind directly to
    // ThreadedEntityClassLoader method because it is not sigc::trackable)
    _defsReloaded = GlobalEntityClassManager().defsReloadedSignal().connect(
        sigc::mem_fun(this, &EntityClassChooser::loadEntityClasses)
    );

    // Setup the tree view and invoke threaded loader to get the entity classes
    setupTreeView();
    loadEntityClasses();

    makeLabelBold(this, "EntityClassChooserDefFileNameLabel");
    makeLabelBold(this, "EntityClassChooserUsageLabel");

    FitToScreen(0.7f, 0.8f);

    wxSplitterWindow* splitter = findNamedObject<wxSplitterWindow>(this, "EntityClassChooserSplitter");

    // Disallow unsplitting
    splitter->SetMinimumPaneSize(200);
    splitter->SetSashPosition(static_cast<int>(GetSize().GetWidth() * 0.2f));

    // Persist layout to registry
    _panedPosition.connect(splitter);
    _panedPosition.loadFromPath(RKEY_SPLIT_POS);

    Bind(wxEVT_CLOSE_WINDOW, &EntityClassChooser::onDeleteEvent, this);

    // Set the model preview height to something significantly smaller than the
    // window's height to allow shrinking
    _modelPreview->getWidget()->SetMinClientSize(
        wxSize(GetSize().GetWidth() * 0.4f, GetSize().GetHeight() * 0.2f));
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

        // Pre-destruction cleanup
        GlobalMainFrame().signal_MainFrameShuttingDown().connect(
            sigc::mem_fun(*instancePtr, &EntityClassChooser::onMainFrameShuttingDown)
        );
    }

    return *instancePtr;
}

EntityClassChooserPtr& EntityClassChooser::InstancePtr()
{
    static EntityClassChooserPtr _instancePtr;
    return _instancePtr;
}

void EntityClassChooser::onMainFrameShuttingDown()
{
    rMessage() << "EntityClassChooser shutting down." << std::endl;

    _modelPreview.reset();
    _defsReloaded.disconnect();

    // Final step at shutdown, release the shared ptr
    Instance().SendDestroyEvent();
    InstancePtr().reset();
}

void EntityClassChooser::loadEntityClasses()
{
    _eclassLoader.reset(new ThreadedEntityClassLoader(_columns, this));
    _eclassLoader->Run();
}

void EntityClassChooser::setSelectedEntityClass(const std::string& eclass)
{
    // Select immediately if possible, otherwise remember class name for later
    // selection
    if (_treeStore != nullptr)
    {
        wxDataViewItem item = _treeStore->FindString(eclass, _columns.name);

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

void EntityClassChooser::onDeleteEvent(wxCloseEvent& ev)
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

    int returnCode = DialogBase::ShowModal();

    _panedPosition.saveToPath(RKEY_SPLIT_POS);

    return returnCode;
}

void EntityClassChooser::setTreeViewModel()
{
    _treeView->AssociateModel(_treeStore.get());

    // Expand the first layer
    _treeView->ExpandTopLevelItems();

    // Pre-select the given class if requested by setSelectedEntityClass()
    if (!_classToHighlight.empty())
    {
        assert(_treeStore);
        setSelectedEntityClass(_classToHighlight);
    }
}

void EntityClassChooser::setupTreeView()
{
    // Use the TreeModel's full string search function
    _treeStore = new wxutil::TreeModel(_columns);
    wxutil::TreeModel::Row row = _treeStore->AddItem();

    row[_columns.name] = wxVariant(wxDataViewIconText(_("Loading...")));

    wxPanel* parent = findNamedObject<wxPanel>(this, "EntityClassChooserLeftPane");

    _treeView = wxutil::TreeView::CreateWithModel(parent, _treeStore);
    _treeView->AddSearchColumn(_columns.name);

    _treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &EntityClassChooser::onSelectionChanged, this);

    // Single column with icon and name
    _treeView->AppendIconTextColumn(_("Classname"), _columns.name.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    parent->GetSizer()->Prepend(_treeView, 1, wxEXPAND | wxBOTTOM | wxRIGHT, 6);
}

// Update the usage information
void EntityClassChooser::updateUsageInfo(const std::string& eclass)
{
    // Lookup the IEntityClass instance
    auto e = GlobalEntityClassManager().findOrInsert(eclass, true);

    // Set the usage panel to the IEntityClass' usage information string
    auto* usageText = findNamedObject<wxTextCtrl>(this, "EntityClassChooserUsageText");
    usageText->SetValue(eclass::getUsage(*e));
}

void EntityClassChooser::updateSelection()
{
    wxDataViewItem item = _treeView->GetSelection();
    auto* defFileName = findNamedObject<wxStaticText>(this, "EntityClassChooserDefFileName");

    if (item.IsOk())
    {
        wxutil::TreeModel::Row row(item, *_treeStore);

        if (!row[_columns.isFolder].getBool())
        {
            // Make the OK button active
            findNamedObject<wxButton>(this, "EntityClassChooserAddButton")->Enable(true);

            // Set the panel text with the usage information
            wxDataViewIconText iconAndName = static_cast<wxDataViewIconText>(row[_columns.name]);
            std::string selName = iconAndName.GetText().ToStdString();

            updateUsageInfo(selName);

            // Update the _selectionName field
            _selectedName = selName;

            // Lookup the IEntityClass instance
            auto eclass = GlobalEntityClassManager().findClass(selName);

            if (eclass)
            {
                _modelPreview->setModel(eclass->getAttribute("model").getValue());
                _modelPreview->setSkin(eclass->getAttribute("skin").getValue());
                defFileName->SetLabel(eclass->getDefFileName());
                return; // success
            }
        }
    }

    // Nothing selected
    _modelPreview->setModel("");
    _modelPreview->setSkin("");
    defFileName->SetLabel("-");

    findNamedObject<wxButton>(this, "EntityClassChooserAddButton")->Enable(false);
}

void EntityClassChooser::onCancel(wxCommandEvent& ev)
{
    _selectedName.clear();

    EndModal(wxID_CANCEL); // break main loop
    Hide();
}

void EntityClassChooser::onOK(wxCommandEvent& ev)
{
    EndModal(wxID_OK); // break main loop
    Hide();
}

void EntityClassChooser::onSelectionChanged(wxDataViewEvent& ev)
{
    updateSelection();
}

void EntityClassChooser::onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev)
{
    _treeView->UnselectAll();

    _treeStore = ev.GetTreeModel();
    setTreeViewModel();
}


} // namespace ui
