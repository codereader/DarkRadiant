#include "FavouritesBrowser.h"

#include "i18n.h"
#include "ifavourites.h"
#include "icommandsystem.h"
#include "iorthoview.h"

#include "wxutil/Bitmap.h"
#include <wx/toolbar.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>

#include "command/ExecutionFailure.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "wxutil/dialog/MessageBox.h"
#include "selectionlib.h"
#include "entitylib.h"

namespace ui
{

namespace
{
    constexpr const char* const APPLY_TEXTURE_TEXT = N_("Apply to selection");
    constexpr const char* const APPLY_TEXTURE_ICON = "textureApplyToSelection16.png";

    constexpr const char* const APPLY_SOUNDSHADER_TEXT = N_("Apply to selection");
    constexpr const char* const APPLY_SOUNDSHADER_ICON = "icon_sound.png";

    constexpr const char* const ADD_ENTITY_TEXT = N_("Create entity");
    constexpr const char* const ADD_ENTITY_ICON = "cmenu_add_entity.png";

    constexpr const char* const ADD_SPEAKER_TEXT = N_("Create speaker");
    constexpr const char* const ADD_SPEAKER_ICON = "icon_sound.png";

    constexpr const char* const REMOVE_FROM_FAVOURITES = N_("Remove from Favourites");
}

FavouritesBrowser::FavouritesBrowser(wxWindow* parent) :
    DockablePanel(parent),
    _listView(nullptr),
    _showFullPath(nullptr),
    _reloadFavouritesOnIdle(true)
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    _listView = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_LIST);
    _listView->Bind(wxEVT_CONTEXT_MENU, &FavouritesBrowser::onContextMenu, this);
    _listView->Bind(wxEVT_LIST_ITEM_ACTIVATED, &FavouritesBrowser::onItemActivated, this);

    _iconList.reset(new wxImageList(16, 16));
    _listView->SetImageList(_iconList.get(), wxIMAGE_LIST_SMALL);

    setupCategories();

    auto* toolHBox = new wxBoxSizer(wxHORIZONTAL);
    toolHBox->Add(createLeftToolBar(), 1, wxEXPAND);
    toolHBox->Add(createRightToolBar(), 0, wxEXPAND);

    GetSizer()->Add(toolHBox, 0, wxEXPAND);
    GetSizer()->Add(_listView, 1, wxEXPAND);

    constructPopupMenu();
}

FavouritesBrowser::~FavouritesBrowser()
{
    if (panelIsActive())
    {
        disconnectListeners();
    }
}

void FavouritesBrowser::onPanelActivated()
{
    connectListeners();
    queueUpdate();
}

void FavouritesBrowser::onPanelDeactivated()
{
    disconnectListeners();
}

void FavouritesBrowser::connectListeners()
{
    // Subscribe to any favourite changes
    for (auto& category : _categories)
    {
        _changedConnections.emplace_back(
            GlobalFavouritesManager().getSignalForType(category.typeName).connect(
                sigc::mem_fun(this, &FavouritesBrowser::onFavouritesChanged)
        ));
    }
}

void FavouritesBrowser::disconnectListeners()
{
    for (auto& connection : _changedConnections)
    {
        connection.disconnect();
    }

    _changedConnections.clear();
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
        std::bind(&FavouritesBrowser::onApplyTextureToSelection, this),
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
        new wxutil::IconTextMenuItem(_(APPLY_SOUNDSHADER_TEXT), APPLY_SOUNDSHADER_ICON),
        std::bind(&FavouritesBrowser::onApplySoundToSelection, this),
        std::bind(&FavouritesBrowser::testApplySoundToSelection, this)
    );

    _popupMenu->addItem(
        new wxutil::StockIconTextMenuItem(_(REMOVE_FROM_FAVOURITES), wxART_DEL_BOOKMARK),
        std::bind(&FavouritesBrowser::onRemoveFromFavourite, this),
        [this]() { return _listView->GetSelectedItemCount() > 0; }
    );
}

void FavouritesBrowser::setupCategories()
{
    _categories.emplace_back(FavouriteCategory{
        decl::getTypeName(decl::Type::Material), _("Materials"), "icon_texture.png",
        _iconList->Add(wxutil::GetLocalBitmap("icon_texture.png")),
        nullptr
    });
    _categories.emplace_back(FavouriteCategory{
        "model", _("Models"), "model16green.png",
        _iconList->Add(wxutil::GetLocalBitmap("icon_model.png")),
        nullptr
    });
    _categories.emplace_back(FavouriteCategory{
        decl::getTypeName(decl::Type::EntityDef), _("EntityDefs"), "cmenu_add_entity.png",
        _iconList->Add(wxutil::GetLocalBitmap("cmenu_add_entity.png")),
        nullptr
    });
    _categories.emplace_back(FavouriteCategory{
        decl::getTypeName(decl::Type::SoundShader), _("Sound Shaders"), "icon_sound.png",
        _iconList->Add(wxutil::GetLocalBitmap("icon_sound.png")),
        nullptr
    });
}

wxToolBar* FavouritesBrowser::createRightToolBar()
{
    auto* toolbar = new wxToolBar(this, wxID_ANY);
    toolbar->SetToolBitmapSize(wxSize(24, 24));

    _showFullPath = new wxCheckBox(toolbar, wxID_ANY, _("Show full Path"));
    _showFullPath->Bind(wxEVT_CHECKBOX, &FavouritesBrowser::onShowFullPathToggled, this);

    toolbar->AddControl(_showFullPath, _showFullPath->GetLabel());

    toolbar->Realize();

    return toolbar;
}

wxToolBar* FavouritesBrowser::createLeftToolBar()
{
    auto* toolbar = new wxToolBar(this, wxID_ANY);
    toolbar->SetToolBitmapSize(wxSize(24, 24));

    for (auto& category : _categories)
    {
        category.checkButton = toolbar->AddCheckTool(wxID_ANY, category.displayName,
            wxutil::GetLocalBitmap(category.iconName, wxART_TOOLBAR));

        category.checkButton->SetShortHelp(category.displayName);
        category.checkButton->Toggle(true);

        toolbar->Bind(wxEVT_TOOL, &FavouritesBrowser::onCategoryToggled, this, category.checkButton->GetId());
    }

    toolbar->Realize();

    return toolbar;
}

void FavouritesBrowser::reloadFavourites()
{
    clearItems();

    for (const auto& category : _categories)
    {
        if (!category.checkButton->IsToggled())
        {
            continue;
        }

        auto favourites = GlobalFavouritesManager().getFavourites(category.typeName);

        for (const auto& fav : favourites)
        {
            auto slashPos = fav.rfind('/');
            auto leafName = fav.substr(slashPos == std::string::npos ? 0 : slashPos + 1);;
            auto displayName = !_showFullPath->IsChecked() ? leafName : fav;

            auto index = _listView->InsertItem(_listView->GetItemCount(), displayName, category.iconIndex);

            // Keep the item info locally, store a pointer to it in the list item user data
            _listItems.emplace_back(FavouriteItem{ category.typeName, fav, leafName });
            _listView->SetItemPtrData(index, reinterpret_cast<wxUIntPtr>(&(_listItems.back())));
        }
    }
}

void FavouritesBrowser::onCategoryToggled(wxCommandEvent& ev)
{
    queueUpdate();
}

void FavouritesBrowser::onShowFullPathToggled(wxCommandEvent& ev)
{
    queueUpdate();
}

void FavouritesBrowser::onFavouritesChanged()
{
    queueUpdate();
}

void FavouritesBrowser::queueUpdate()
{
    _reloadFavouritesOnIdle = true;
    requestIdleCallback();
}

void FavouritesBrowser::onIdle()
{
    if (_reloadFavouritesOnIdle)
    {
        _reloadFavouritesOnIdle = false;
        reloadFavourites();
    }
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

    if (data->typeName == decl::getTypeName(decl::Type::Material))
    {
        onApplyTextureToSelection();
    }
    else if (data->typeName == decl::getTypeName(decl::Type::EntityDef))
    {
        onCreateEntity();
    }
    else if (data->typeName == decl::getTypeName(decl::Type::SoundShader))
    {
        if (testCreateSpeaker())
        {
            onCreateSpeaker();
        }
        else if (testApplySoundToSelection())
        {
            onApplySoundToSelection();
        }
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

std::string FavouritesBrowser::getSelectedTypeName()
{
    auto selection = getSelectedItems();

    if (selection.size() != 1) return "";

    auto* data = reinterpret_cast<FavouriteItem*>(_listView->GetItemData(selection.front()));
    return data->typeName;
}

void FavouritesBrowser::onRemoveFromFavourite()
{
    auto selection = getSelectedItems();

    for (auto itemIndex : selection)
    {
        auto* data = reinterpret_cast<FavouriteItem*>(_listView->GetItemData(itemIndex));
        GlobalFavouritesManager().removeFavourite(data->typeName, data->fullPath);
    }

    // A repopulation has already been rescheduled
}

void FavouritesBrowser::onApplyTextureToSelection()
{
    auto selection = getSelectedItems();

    if (selection.size() != 1) return;

    auto* data = reinterpret_cast<FavouriteItem*>(_listView->GetItemData(selection.front()));

    GlobalCommandSystem().executeCommand("SetShaderOnSelection", data->fullPath);
}

bool FavouritesBrowser::testSingleTextureSelected()
{
    return getSelectedTypeName() == decl::getTypeName(decl::Type::Material);
}

void FavouritesBrowser::onApplySoundToSelection()
{
    auto selection = getSelectedItems();

    if (selection.size() != 1) return;

    auto* data = reinterpret_cast<FavouriteItem*>(_listView->GetItemData(selection.front()));

    UndoableCommand cmd("ApplySoundShaderToSelection");

    scene::foreachSelectedEntity([&](Entity& entity)
    {
        entity.setKeyValue("s_shader", data->leafName);
    });
}

bool FavouritesBrowser::testApplySoundToSelection()
{
    const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

    return info.entityCount > 0 && getSelectedTypeName() == decl::getTypeName(decl::Type::SoundShader);
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
    if (getSelectedTypeName() != decl::getTypeName(decl::Type::EntityDef))
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
        cmd::Argument(data->leafName), cmd::Argument(GlobalXYWndManager().getActiveViewOrigin())
    });
}

bool FavouritesBrowser::testCreateSpeaker()
{
    if (getSelectedTypeName() != decl::getTypeName(decl::Type::SoundShader))
    {
        return false;
    }

    return selectionAllowsEntityCreation();
}

}
