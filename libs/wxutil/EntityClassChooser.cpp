#include "EntityClassChooser.h"

#include <stdexcept>
#include "dataview/TreeModel.h"
#include "dataview/TreeViewItemStyle.h"
#include "dataview/ThreadedResourceTreePopulator.h"
#include "dataview/ResourceTreeViewToolbar.h"
#include "dataview/VFSTreePopulator.h"
#include "decl/DeclarationSelector.h"

#include "i18n.h"
#include "ifavourites.h"
#include "ideclmanager.h"
#include "ui/iuserinterface.h"
#include "gamelib.h"

#include <wx/button.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include "wxutil/Bitmap.h"
#include "wxutil/Icon.h"

#include "eclass.h"

#include "debugging/ScopedDebugTimer.h"
#include "preview/EntityClassPreview.h"

namespace wxutil
{

namespace
{
    constexpr const char* const TITLE_ADD_ENTITY = N_("Create Entity");
    constexpr const char* const TITLE_CONVERT_TO_ENTITY = N_("Convert to Entity");
    constexpr const char* const TITLE_SELECT_ENTITY = N_("Select Entity Class");
    constexpr const char* const RKEY_SPLIT_POS = "user/ui/entityClassChooser/splitPos";
    constexpr const char* const RKEY_WINDOW_STATE = "user/ui/entityClassChooser/window";
    constexpr const char* const RKEY_LAST_SELECTED_ECLASS = "user/ui/entityClassChooser/lastSelectedEclass";

    constexpr const char* const FOLDER_ICON = "folder16.png";
    constexpr const char* const ENTITY_ICON = "cmenu_add_entity.png";

    // Registry XPath to lookup key that specifies the display folder
    constexpr const char* const FOLDER_KEY_PATH = "/entityChooser/displayFolderKey";

    std::string getDialogTitle(EntityClassChooser::Purpose purpose)
    {
        switch (purpose)
        {
        case EntityClassChooser::Purpose::AddEntity: return _(TITLE_ADD_ENTITY);
        case EntityClassChooser::Purpose::ConvertEntity: return _(TITLE_CONVERT_TO_ENTITY);
        case EntityClassChooser::Purpose::SelectClassname: return _(TITLE_SELECT_ENTITY);
        default:
            throw std::logic_error("Unknown entity class chooser purpose");
        }
    }
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
    const DeclarationTreeView::Columns& _columns;

    // Key that specifies the display folder
    std::string _folderKey;

    wxutil::Icon _folderIcon;
    wxutil::Icon _entityIcon;

    std::set<std::string> _favourites;

public:

    // Constructor
    EntityClassTreePopulator(const TreeModel::Ptr& store,
                             const DeclarationTreeView::Columns& columns)
    : VFSTreePopulator(store),
      _store(store),
      _columns(columns),
      _folderKey(game::current::getValue<std::string>(FOLDER_KEY_PATH)),
      _folderIcon(GetLocalBitmap(FOLDER_ICON)),
      _entityIcon(GetLocalBitmap(ENTITY_ICON))
    {
        // Get the list of favourite eclasses
        _favourites = GlobalFavouritesManager().getFavourites(decl::getTypeName(decl::Type::EntityDef));
    }

    // EntityClassVisitor implementation
    void visit(const IEntityClassPtr& eclass)
    {
        // Skip hidden entity classes
        if (eclass->getVisibility() == vfs::Visibility::HIDDEN)
            return;

        std::string folderPath = eclass->getAttributeValue(_folderKey);
        if (!folderPath.empty())
        {
            folderPath = "/" + folderPath;
        }

        // Create the folder to put this EntityClass in, depending on the value
        // of the DISPLAY_FOLDER_KEY.
        addPath(
            eclass->getModName() + folderPath + "/" + eclass->getDeclName(),
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
                row[_columns.declName] = eclass->getDeclName();

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
    const DeclarationTreeView::Columns& _columns;

public:
    ThreadedEntityClassLoader(const DeclarationTreeView::Columns& cols) :
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

class EntityClassDescription :
    public wxPanel,
    public ui::IDeclarationPreview
{
private:
    wxTextCtrl* _textCtrl;
public:
    EntityClassDescription(wxWindow* parent) :
        wxPanel(parent)
    {
        SetSizer(new wxBoxSizer(wxVERTICAL));

        _textCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
            wxSize(-1, 90), wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
        _textCtrl->SetMinSize(wxSize(-1, 90));

        auto descriptionLabel = new wxStaticText(this, wxID_ANY, _("Description"));
        descriptionLabel->SetFont(descriptionLabel->GetFont().Bold());

        GetSizer()->Add(descriptionLabel, 0, wxEXPAND | wxALIGN_LEFT, 0);
        GetSizer()->Add(_textCtrl, 1, wxEXPAND | wxTOP, 6);

        Disable();
    }

    wxWindow* GetPreviewWidget() override
    {
        return this;
    }

    void ClearPreview() override
    {
        _textCtrl->SetValue("");
        Enable(false);
    }

    void SetPreviewDeclName(const std::string& declName) override
    {
        // Lookup the IEntityClass instance
        auto eclass = GlobalEntityClassManager().findClass(declName);
        _textCtrl->SetValue(eclass ? eclass::getUsage(eclass) : "");

        Enable(!declName.empty() && eclass);
    }
};

class EntityClassSelector :
    public DeclarationSelector
{
private:
    // Model preview widget
    std::unique_ptr<EntityClassPreview> _preview;

public:
    EntityClassSelector(wxWindow* parent) :
        DeclarationSelector(parent, decl::Type::EntityDef),
        _preview(new EntityClassPreview(this))
    {
        GetTreeView()->SetExpandTopLevelItemsAfterPopulation(true);

        AddPreviewToRightPane(_preview.get());
        AddPreviewToBottom(new EntityClassDescription(this));
    }

    void LoadEntityClasses()
    {
        PopulateTreeView(std::make_shared<ThreadedEntityClassLoader>(GetColumns()));
    }
};

EntityClassChooser::EntityClassChooser(Purpose purpose) :
    DialogBase(getDialogTitle(purpose)),
    _selector(nullptr)
{
    loadNamedPanel(this, "EntityClassChooserMainPanel");

    // Connect button signals
    auto confirmButton = findNamedObject<wxButton>(this, "EntityClassChooserAddButton");
    confirmButton->Bind(wxEVT_BUTTON, &EntityClassChooser::onOK, this);

    switch (purpose)
    {
    case Purpose::AddEntity: 
        confirmButton->SetBitmap(wxArtProvider::GetBitmap(wxART_PLUS));
        break;
    case Purpose::ConvertEntity:
        confirmButton->SetLabelText(_("Convert"));
        break;
    case Purpose::SelectClassname:
        confirmButton->SetLabelText(_("Select"));
        break;
    default:
        throw std::logic_error("Unknown entity class chooser purpose");
    }

    findNamedObject<wxButton>(this, "EntityClassChooserCancelButton")->Bind(
        wxEVT_BUTTON, &EntityClassChooser::onCancel, this);

    // Listen for defs-reloaded signal (cannot bind directly to
    // ThreadedEntityClassLoader method because it is not sigc::trackable)
    _defsReloaded = GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::EntityDef).connect(
        [this]() { GlobalUserInterface().dispatch([this]() { loadEntityClasses(); }); }
    );

    // Setup the tree view and invoke threaded loader to get the entity classes
    setupSelector();
    loadEntityClasses();

#if 0
    // Disallow unsplitting
    splitter->SetMinimumPaneSize(200);
    splitter->SetSashPosition(static_cast<int>(GetSize().GetWidth() * 0.2f));
#endif
    // Persist layout to registry
    _windowPosition.initialise(this, RKEY_WINDOW_STATE, 0.7f, 0.8f);
#if 0
    _panedPosition.connect(splitter);
    _panedPosition.loadFromPath(RKEY_SPLIT_POS);
#endif
    Bind(wxEVT_CLOSE_WINDOW, &EntityClassChooser::onDeleteEvent, this);
#if 0
    // Set the model preview height to something significantly smaller than the
    // window's height to allow shrinking
    _modelPreview->getWidget()->SetMinClientSize(
        wxSize(GetSize().GetWidth() * 0.4f, GetSize().GetHeight() * 0.2f));
#endif
}

EntityClassChooser::~EntityClassChooser()
{
    _defsReloaded.disconnect();
}

std::string EntityClassChooser::ChooseEntityClass(Purpose purpose, const std::string& eclassToSelect)
{
    EntityClassChooser instance{ purpose };

    // Fall back to the value we saved in the registry if we didn't get any other instructions
    auto preselectEclass = !eclassToSelect.empty() ? eclassToSelect : 
        registry::getValue<std::string>(RKEY_LAST_SELECTED_ECLASS);

    if (!preselectEclass.empty())
    {
        instance.setSelectedEntityClass(preselectEclass);
    }

    if (instance.ShowModal() == wxID_OK)
    {
        auto selection = instance.getSelectedEntityClass();

        // Remember this selection on OK
        if (!selection.empty())
        {
            registry::setValue(RKEY_LAST_SELECTED_ECLASS, selection);
        }

        return selection;
    }
    
    return ""; // Empty selection on cancel
}

void EntityClassChooser::loadEntityClasses()
{
    _selector->LoadEntityClasses();
}

void EntityClassChooser::setSelectedEntityClass(const std::string& eclass)
{
    _selector->SetSelectedDeclName(eclass);
}

std::string EntityClassChooser::getSelectedEntityClass() const
{
    return _selector->GetSelectedDeclName();
}

void EntityClassChooser::onDeleteEvent(wxCloseEvent& ev)
{
    EndModal(wxID_CANCEL); // break main loop
}

int EntityClassChooser::ShowModal()
{
    _windowPosition.applyPosition();

    // Update the member variables
    updateSelection();

    _selector->FocusTreeView();

    int returnCode = DialogBase::ShowModal();

    _panedPosition.saveToPath(RKEY_SPLIT_POS);
    _windowPosition.saveToPath(RKEY_WINDOW_STATE);

    return returnCode;
}

void EntityClassChooser::setupSelector()
{
    auto parent = findNamedObject<wxPanel>(this, "EntityClassChooserLeftPane");

    _selector = new EntityClassSelector(parent);

    _selector->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &EntityClassChooser::onSelectionChanged, this);
    _selector->Bind( wxEVT_DATAVIEW_ITEM_ACTIVATED, &EntityClassChooser::_onItemActivated, this );

    parent->GetSizer()->Prepend(_selector, 1, wxEXPAND | wxBOTTOM | wxRIGHT, 6);
}

void EntityClassChooser::_onItemActivated( wxDataViewEvent& ev )
{
    auto selectedEclass = _selector->GetSelectedDeclName();

    if (!selectedEclass.empty())
    {
        onOK(ev);
    }
}

void EntityClassChooser::updateSelection()
{
    auto selectedEclass = _selector->GetSelectedDeclName();

    if (selectedEclass.empty())
    {
        findNamedObject<wxButton>(this, "EntityClassChooserAddButton")->Enable(false);
        return;
    }

    // Make the OK button active
    findNamedObject<wxButton>(this, "EntityClassChooserAddButton")->Enable(true);
}

void EntityClassChooser::onCancel(wxCommandEvent& ev)
{
    EndModal(wxID_CANCEL); // break main loop
}

void EntityClassChooser::onOK(wxCommandEvent& ev)
{
    EndModal(wxID_OK); // break main loop
}

void EntityClassChooser::onSelectionChanged(wxDataViewEvent& ev)
{
    updateSelection();
}

} // namespace ui
