#include "FavouritesBrowser.h"

#include "i18n.h"
#include "ifavourites.h"
#include "igroupdialog.h"
#include "iuimanager.h"
#include "imainframe.h"
#include "icommandsystem.h"

#include <wx/artprov.h>
#include <wx/toolbar.h>
#include <wx/checkbox.h>
#include <wx/frame.h>
#include <wx/sizer.h>

#include "module/StaticModule.h"
#include "command/ExecutionFailure.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "wxutil/dialog/MessageBox.h"
#include "iorthoview.h"
#include "selectionlib.h"

namespace ui
{

namespace
{
    const char* const TAB_NAME = "favourites";

    const char* const APPLY_TEXTURE_TEXT = N_("Apply to selection");
    const char* const APPLY_TEXTURE_ICON = "textureApplyToSelection16.png";

    const char* const ADD_ENTITY_TEXT = N_("Create entity");
    const char* const ADD_ENTITY_ICON = "cmenu_add_entity.png";

    const char* const ADD_SPEAKER_TEXT = N_("Create speaker");
    const char* const ADD_SPEAKER_ICON = "icon_sound.png";

    const char* const REMOVE_FROM_FAVOURITES = N_("Remove from Favourites");
}

FavouritesBrowser::FavouritesBrowser() :
    _tempParent(nullptr),
    _mainWidget(nullptr),
    _listView(nullptr),
    _showFullPath(nullptr),
    _updateNeeded(true)
{}

void FavouritesBrowser::construct()
{
    if (_mainWidget != nullptr)
    {
        return;
    }

    _tempParent = new wxFrame(nullptr, wxID_ANY, "");
    _tempParent->Hide();

    _mainWidget = new wxPanel(_tempParent, wxID_ANY);
    _mainWidget->SetSizer(new wxBoxSizer(wxVERTICAL));

    _listView = new wxListView(_mainWidget, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_LIST);
    _listView->Bind(wxEVT_PAINT, &FavouritesBrowser::onListCtrlPaint, this);
    _listView->Bind(wxEVT_CONTEXT_MENU, &FavouritesBrowser::onContextMenu, this);
    _listView->Bind(wxEVT_LIST_ITEM_ACTIVATED, &FavouritesBrowser::onItemActivated, this);

    _iconList.reset(new wxImageList(16, 16));
    _listView->SetImageList(_iconList.get(), wxIMAGE_LIST_SMALL);

    setupCategories();
    
    auto* toolHBox = new wxBoxSizer(wxHORIZONTAL);
    toolHBox->Add(createLeftToolBar(), 1, wxEXPAND);
    toolHBox->Add(createRightToolBar(), 0, wxEXPAND);

    _mainWidget->GetSizer()->Add(toolHBox, 0, wxEXPAND);
    _mainWidget->GetSizer()->Add(_listView, 1, wxEXPAND);

    constructPopupMenu();
}

void FavouritesBrowser::clearItems()
{
    _listView->ClearAll();
    _listItems.clear();
}

void FavouritesBrowser::constructPopupMenu()
{
    _popupMenu.reset(new wxutil::PopupMenu);

    _popupMenu->addItem(
        new wxutil::IconTextMenuItem(_(APPLY_TEXTURE_TEXT), APPLY_TEXTURE_ICON),
        std::bind(&FavouritesBrowser::onApplyToSelection, this),
        std::bind(&FavouritesBrowser::testSingleTextureSelected, this)
    );

    _popupMenu->addItem(
        new wxutil::IconTextMenuItem(_(ADD_ENTITY_TEXT), ADD_ENTITY_ICON),
        std::bind(&FavouritesBrowser::onCreateEntity, this),
        std::bind(&FavouritesBrowser::testCreateEntity, this)
    );

    _popupMenu->addItem(
        new wxutil::IconTextMenuItem(_(ADD_SPEAKER_TEXT), ADD_SPEAKER_ICON),
        std::bind(&FavouritesBrowser::onCreateSpeaker, this),
        std::bind(&FavouritesBrowser::testCreateSpeaker, this)
    );

    _popupMenu->addItem(
        new wxutil::StockIconTextMenuItem(_(REMOVE_FROM_FAVOURITES), wxART_DEL_BOOKMARK),
        std::bind(&FavouritesBrowser::onRemoveFromFavourite, this),
        [this]() { return _listView->GetSelectedItemCount() > 0; }
    );
}

void FavouritesBrowser::setupCategories()
{
    auto prefix = GlobalUIManager().ArtIdPrefix();

    _categories.emplace_back(FavouriteCategory{
        decl::Type::Material, _("Materials"), "icon_texture.png",
        _iconList->Add(wxArtProvider::GetBitmap(prefix + "icon_texture.png")),
        nullptr
    });
    _categories.emplace_back(FavouriteCategory{
        decl::Type::Model, _("Models"), "icon_model.png",
        _iconList->Add(wxArtProvider::GetBitmap(prefix + "icon_model.png")),
        nullptr
    });
    _categories.emplace_back(FavouriteCategory{
        decl::Type::EntityDef, _("EntityDefs"), "cmenu_add_entity.png",
        _iconList->Add(wxArtProvider::GetBitmap(prefix + "cmenu_add_entity.png")),
        nullptr
    });
    _categories.emplace_back(FavouriteCategory{
        decl::Type::SoundShader, _("Sound Shaders"), "icon_sound.png",
        _iconList->Add(wxArtProvider::GetBitmap(prefix + "icon_sound.png")),
        nullptr
    });

    // Subscribe to any favourite changes
    for (auto& category : _categories)
    {
        changedConnections.emplace_back(GlobalFavouritesManager().getSignalForType(category.type).connect(
            sigc::mem_fun(this, &FavouritesBrowser::onFavouritesChanged)
        ));
    }
}

wxToolBar* FavouritesBrowser::createRightToolBar()
{
    auto* toolbar = new wxToolBar(_mainWidget, wxID_ANY);
    toolbar->SetToolBitmapSize(wxSize(24, 24));

    _showFullPath = new wxCheckBox(toolbar, wxID_ANY, _("Show full Path"));
    _showFullPath->Bind(wxEVT_CHECKBOX, &FavouritesBrowser::onShowFullPathToggled, this);

    toolbar->AddControl(_showFullPath, _showFullPath->GetLabel());

    toolbar->Realize();

    return toolbar;
}

wxToolBar* FavouritesBrowser::createLeftToolBar()
{
    auto* toolbar = new wxToolBar(_mainWidget, wxID_ANY);
    toolbar->SetToolBitmapSize(wxSize(24, 24));

    for (auto& category : _categories)
    {
        category.checkButton = toolbar->AddCheckTool(wxID_ANY, category.displayName,
            wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + category.iconName, wxART_TOOLBAR));

        category.checkButton->SetShortHelp(category.displayName);
        category.checkButton->Toggle(true);

        toolbar->Bind(wxEVT_TOOL, &FavouritesBrowser::onCategoryToggled, this, category.checkButton->GetId());
    }

    toolbar->Realize();

    return toolbar;
}

void FavouritesBrowser::reloadFavourites()
{
    _updateNeeded = false;
    clearItems();

    for (const auto& category : _categories)
    {
        if (!category.checkButton->IsToggled())
        {
            continue;
        }

        auto favourites = GlobalFavouritesManager().getFavourites(category.type);

        for (const auto& fav : favourites)
        {
            auto displayName = fav;

            if (!_showFullPath->IsChecked())
            {
                auto slashPos = displayName.rfind('/');
                displayName = displayName.substr(slashPos == std::string::npos ? 0 : slashPos + 1);
            }

            auto index = _listView->InsertItem(_listView->GetItemCount(), displayName, category.iconIndex);

            // Keep the item info locally, store a pointer to it in the list item user data
            _listItems.emplace_back(FavouriteItem{ category.type, fav });
            _listView->SetItemPtrData(index, reinterpret_cast<wxUIntPtr>(&(_listItems.back())));
        }
    }
}

const std::string& FavouritesBrowser::getName() const
{
    static std::string _name("FavouritesBrowser");
    return _name;
}

const StringSet& FavouritesBrowser::getDependencies() const
{
    static StringSet _dependencies;

    if (_dependencies.empty())
    {
        _dependencies.insert(MODULE_MAINFRAME);
        _dependencies.insert(MODULE_COMMANDSYSTEM);
        _dependencies.insert(MODULE_FAVOURITES_MANAGER);
    }

    return _dependencies;
}

void FavouritesBrowser::initialiseModule(const IApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;

    GlobalCommandSystem().addCommand("ToggleFavouritesBrowser", 
        sigc::mem_fun(this, &FavouritesBrowser::togglePage));

    construct();

    // The startup event will add this page to the group dialog tab
    GlobalMainFrame().signal_MainFrameConstructed().connect(
        sigc::mem_fun(*this, &FavouritesBrowser::onMainFrameConstructed)
    );

    _updateNeeded = true;
}

void FavouritesBrowser::shutdownModule()
{
    _iconList.reset();

    for (auto& connection : changedConnections)
    {
        connection.disconnect();
    }
}

void FavouritesBrowser::togglePage(const cmd::ArgumentList& args)
{
    GlobalGroupDialog().togglePage(TAB_NAME);
}

void FavouritesBrowser::onMainFrameConstructed()
{
    // Add the Media Browser page
    auto page = std::make_shared<IGroupDialog::Page>();

    page->name = TAB_NAME;
    page->windowLabel = _("Favourites");
    page->page = _mainWidget;
    page->tabIcon = "favourite.png";
    page->tabLabel = _("Favourites");
    page->position = IGroupDialog::Page::Position::Favourites;

    GlobalGroupDialog().addPage(page);

    if (_tempParent != nullptr)
    {
        _tempParent->Destroy();
        _tempParent = nullptr;
    }
}

void FavouritesBrowser::onCategoryToggled(wxCommandEvent& ev)
{
    reloadFavourites();
}

void FavouritesBrowser::onShowFullPathToggled(wxCommandEvent& ev)
{
    reloadFavourites();
}

void FavouritesBrowser::onFavouritesChanged()
{
    _updateNeeded = true; // Update next time we get painted
}

void FavouritesBrowser::onListCtrlPaint(wxPaintEvent& ev)
{
    if (_updateNeeded)
    {
        reloadFavourites();
    }

    ev.Skip();
}

void FavouritesBrowser::onContextMenu(wxContextMenuEvent& ev)
{
    _popupMenu->show(_listView);
}

void FavouritesBrowser::onItemActivated(wxListEvent& ev)
{
    auto selection = getSelectedItems();

    if (selection.size() != 1) return;

    auto* data = reinterpret_cast<FavouriteItem*>(_listView->GetItemData(selection.front()));
    
    switch (data->type)
    {
    case decl::Type::Material:
        onApplyToSelection();
        break;
    case decl::Type::EntityDef:
        onCreateEntity();
        break;
    case decl::Type::SoundShader:
        onCreateSpeaker();
        break;
    }
}

std::vector<long> FavouritesBrowser::getSelectedItems()
{
    std::vector<long> list;

    long item = _listView->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

    while (item != -1)
    {
        list.push_back(item);

        item = _listView->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }

    return list;
}

decl::Type FavouritesBrowser::getSelectedDeclType()
{
    auto selection = getSelectedItems();

    if (selection.size() != 1) return decl::Type::None;

    auto* data = reinterpret_cast<FavouriteItem*>(_listView->GetItemData(selection.front()));
    return data->type;
}

void FavouritesBrowser::onRemoveFromFavourite()
{
    auto selection = getSelectedItems();

    for (auto itemIndex : selection)
    {
        auto* data = reinterpret_cast<FavouriteItem*>(_listView->GetItemData(itemIndex));
        GlobalFavouritesManager().removeFavourite(data->type, data->fullPath);
    }

    // A repopulation has already been rescheduled
}

void FavouritesBrowser::onApplyToSelection()
{
    auto selection = getSelectedItems();

    if (selection.size() != 1) return;

    auto* data = reinterpret_cast<FavouriteItem*>(_listView->GetItemData(selection.front()));
    
    GlobalCommandSystem().executeCommand("SetShaderOnSelection", data->fullPath);
}

bool FavouritesBrowser::testSingleTextureSelected()
{
    return getSelectedDeclType() == decl::Type::Material;
}

void FavouritesBrowser::onCreateEntity()
{
    if (!testCreateEntity()) return;

    auto selection = getSelectedItems();

    if (selection.size() != 1) return;

    auto* data = reinterpret_cast<FavouriteItem*>(_listView->GetItemData(selection.front()));

    UndoableCommand command("createEntity");

    // Create the entity. We might get an EntityCreationException if the
    // wrong number of brushes is selected.
    try
    {
        GlobalEntityModule().createEntityFromSelection(data->fullPath, GlobalXYWndManager().getActiveViewOrigin());
    }
    catch (cmd::ExecutionFailure& e)
    {
        wxutil::Messagebox::ShowError(e.what());
    }
}

bool FavouritesBrowser::testCreateEntity()
{
    if (getSelectedDeclType() != decl::Type::EntityDef)
    {
        return false;
    }

    return selectionAllowsEntityCreation();
}

bool FavouritesBrowser::selectionAllowsEntityCreation()
{
    const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

    bool anythingSelected = info.totalCount > 0;
    bool noEntities = info.entityCount == 0;
    bool noComponents = info.componentCount == 0;
    bool onlyPrimitivesSelected = anythingSelected && noEntities && noComponents;

    return !anythingSelected || onlyPrimitivesSelected;
}

void FavouritesBrowser::onCreateSpeaker()
{
    if (!testCreateSpeaker()) return;

    auto selection = getSelectedItems();

    if (selection.size() != 1) return;

    auto* data = reinterpret_cast<FavouriteItem*>(_listView->GetItemData(selection.front()));

    GlobalCommandSystem().executeCommand("CreateSpeaker", {
        cmd::Argument(data->fullPath), cmd::Argument(GlobalXYWndManager().getActiveViewOrigin())
    });
}

bool FavouritesBrowser::testCreateSpeaker()
{
    if (getSelectedDeclType() != decl::Type::SoundShader)
    {
        return false;
    }

    return selectionAllowsEntityCreation();
}

module::StaticModule<FavouritesBrowser> favouritesBrowserModule;

}
