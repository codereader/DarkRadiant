#include "EntityClassChooser.h"

#include "dataview/TreeModel.h"
#include "dataview/TreeViewItemStyle.h"
#include "dataview/ThreadedResourceTreePopulator.h"
#include "dataview/ResourceTreeViewToolbar.h"
#include "dataview/VFSTreePopulator.h"

#include "i18n.h"
#include "ifavourites.h"
#include "imainframe.h"
#include "gamelib.h"

#include <wx/button.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include "wxutil/Bitmap.h"

#include "string/string.h"
#include "eclass.h"

#include "debugging/ScopedDebugTimer.h"

namespace wxutil
{

namespace
{
    const char* const ECLASS_CHOOSER_TITLE = N_("Create entity");
    const char* const RKEY_SPLIT_POS = "user/ui/entityClassChooser/splitPos";
    const char* const RKEY_WINDOW_STATE = "user/ui/entityClassChooser/window";

    const char* const FOLDER_ICON = "folder16.png";
    const char* const ENTITY_ICON = "cmenu_add_entity.png";

    // Registry XPath to lookup key that specifies the display folder
    const char* const FOLDER_KEY_PATH = "/entityChooser/displayFolderKey";
}

/*
 * EntityClassVisitor which populates a treeStore with entity classnames
 * taking account of display folders and mod names.
 */
class EntityClassTreePopulator:
    public VFSTreePopulator,
    public EntityClassVisitor
{
    // TreeStore to populate
    TreeModel::Ptr _store;

    // Column definition
    const ResourceTreeView::Columns& _columns;

    // Key that specifies the display folder
    std::string _folderKey;

    wxIcon _folderIcon;
    wxIcon _entityIcon;

    std::set<std::string> _favourites;

public:

    // Constructor
    EntityClassTreePopulator(const TreeModel::Ptr& store,
                             const ResourceTreeView::Columns& columns)
    : VFSTreePopulator(store),
      _store(store),
      _columns(columns),
      _folderKey(game::current::getValue<std::string>(FOLDER_KEY_PATH))
    {
        _folderIcon.CopyFromBitmap(wxutil::GetLocalBitmap(FOLDER_ICON));
        _entityIcon.CopyFromBitmap(wxutil::GetLocalBitmap(ENTITY_ICON));

        // Get the list of favourite eclasses
        _favourites = GlobalFavouritesManager().getFavourites(decl::Type::EntityDef);
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
            [&](TreeModel::Row& row, const std::string& path,
                const std::string& leafName, bool isFolder)
            {
                bool isFavourite = !isFolder && _favourites.count(leafName) > 0;

                // Get the display name by stripping off everything before the
                // last slash
                row[_columns.iconAndName] = wxVariant(
                    wxDataViewIconText(leafName, !isFolder ? _entityIcon : _folderIcon)
                );
                row[_columns.fullName] = leafName;
                row[_columns.leafName] = leafName;

                row[_columns.isFolder] = isFolder;
                row[_columns.isFavourite] = isFavourite;
                row[_columns.iconAndName] = TreeViewItemStyle::Declaration(isFavourite); // assign attributes
                row.SendItemAdded();
            }
        );
    }
};

// Local class for loading entity class definitions in a separate thread
class ThreadedEntityClassLoader final :
    public ThreadedResourceTreePopulator
{
private:
    // Column specification struct
    const ResourceTreeView::Columns& _columns;

public:
    ThreadedEntityClassLoader(const ResourceTreeView::Columns& cols) :
        ThreadedResourceTreePopulator(cols),
        _columns(cols)
    {}

    ~ThreadedEntityClassLoader()
    {
        EnsureStopped();
    }

    void PopulateModel(const TreeModel::Ptr& model) override
    {
        // Populate it with the list of entity classes by using a visitor class.
        EntityClassTreePopulator visitor(model, _columns);
        GlobalEntityClassManager().forEachEntityClass(visitor);
    }

    void SortModel(const TreeModel::Ptr& model) override
    {
        model->SortModelFoldersFirst(_columns.leafName, _columns.isFolder);
    }
};

// Main constructor
EntityClassChooser::EntityClassChooser() :
    DialogBase(_(ECLASS_CHOOSER_TITLE)),
    _treeView(nullptr),
    _selectedName("")
{
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

    _modelPreview.reset(new ModelPreview(rightPanel));

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

    wxSplitterWindow* splitter = findNamedObject<wxSplitterWindow>(this, "EntityClassChooserSplitter");

    // Disallow unsplitting
    splitter->SetMinimumPaneSize(200);
    splitter->SetSashPosition(static_cast<int>(GetSize().GetWidth() * 0.2f));

    // Persist layout to registry
    _windowPosition.initialise(this, RKEY_WINDOW_STATE, 0.7f, 0.8f);

    _panedPosition.connect(splitter);
    _panedPosition.loadFromPath(RKEY_SPLIT_POS);

    Bind(wxEVT_CLOSE_WINDOW, &EntityClassChooser::onDeleteEvent, this);

    // Set the model preview height to something significantly smaller than the
    // window's height to allow shrinking
    _modelPreview->getWidget()->SetMinClientSize(
        wxSize(GetSize().GetWidth() * 0.4f, GetSize().GetHeight() * 0.2f));
}

EntityClassChooser::~EntityClassChooser()
{
    _defsReloaded.disconnect();
}

// Display the singleton instance
std::string EntityClassChooser::chooseEntityClass(const std::string& preselectEclass)
{
    EntityClassChooser instance;

    if (!preselectEclass.empty())
    {
        instance.setSelectedEntityClass(preselectEclass);
    }

    if (instance.ShowModal() == wxID_OK)
    {
        return instance.getSelectedEntityClass();
    }
    else
    {
        return ""; // Empty selection on cancel
    }
}

void EntityClassChooser::loadEntityClasses()
{
    _treeView->Populate(std::make_shared<ThreadedEntityClassLoader>(_columns));
}

void EntityClassChooser::setSelectedEntityClass(const std::string& eclass)
{
    _treeView->SetSelectedFullname(eclass);
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
    _windowPosition.applyPosition();

    _treeViewToolbar->ClearFilter();

    // Update the member variables
    updateSelection();

    // Focus on the treeview
    _treeView->SetFocus();

    int returnCode = DialogBase::ShowModal();

    _panedPosition.saveToPath(RKEY_SPLIT_POS);
    _windowPosition.saveToPath(RKEY_WINDOW_STATE);

    return returnCode;
}

void EntityClassChooser::setupTreeView()
{
    wxPanel* parent = findNamedObject<wxPanel>(this, "EntityClassChooserLeftPane");

    _treeView = new ResourceTreeView(parent, _columns, wxDV_NO_HEADER);
    _treeView->AddSearchColumn(_columns.iconAndName);
    _treeView->SetExpandTopLevelItemsAfterPopulation(true);
    _treeView->EnableFavouriteManagement(decl::Type::EntityDef);

    _treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &EntityClassChooser::onSelectionChanged, this);

    // Single column with icon and name
    _treeView->AppendIconTextColumn(_("Classname"), _columns.iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    _treeViewToolbar = new ResourceTreeViewToolbar(parent, _treeView);

    parent->GetSizer()->Prepend(_treeView, 1, wxEXPAND | wxBOTTOM | wxRIGHT, 6);
    parent->GetSizer()->Prepend(_treeViewToolbar, 0, wxEXPAND | wxALIGN_LEFT | wxBOTTOM | wxLEFT | wxRIGHT, 6);
}

// Update the usage information
void EntityClassChooser::updateUsageInfo(const std::string& eclass)
{
    // Lookup the IEntityClass instance
    auto e = GlobalEntityClassManager().findOrInsert(eclass, true);

    // Set the usage panel to the IEntityClass' usage information string
    auto* usageText = findNamedObject<wxTextCtrl>(this, "EntityClassChooserUsageText");
    usageText->SetValue(e ? eclass::getUsage(*e) : "");
}

void EntityClassChooser::updateSelection()
{
    wxDataViewItem item = _treeView->GetSelection();
    auto* defFileName = findNamedObject<wxStaticText>(this, "EntityClassChooserDefFileName");

    if (item.IsOk())
    {
        TreeModel::Row row(item, *_treeView->GetModel());

        if (!row[_columns.isFolder].getBool())
        {
            // Make the OK button active
            findNamedObject<wxButton>(this, "EntityClassChooserAddButton")->Enable(true);

            // Set the panel text with the usage information
            std::string selName = row[_columns.leafName];

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

} // namespace ui
