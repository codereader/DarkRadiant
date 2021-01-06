#include "FavouritesBrowser.h"

#include "i18n.h"
#include "ifavourites.h"
#include "igroupdialog.h"
#include "iuimanager.h"

#include <wx/artprov.h>
#include <wx/toolbar.h>
#include <wx/checkbox.h>

#include "module/StaticModule.h"

namespace ui
{

namespace
{
    const char* const TAB_NAME = "favourites";
}

FavouritesBrowser::FavouritesBrowser() :
    _tempParent(nullptr),
    _mainWidget(nullptr),
    _listView(nullptr)
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

    _listView = new wxListCtrl(_mainWidget, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_LIST);

    _iconList.reset(new wxImageList(16, 16));
    _listView->SetImageList(_iconList.get(), wxIMAGE_LIST_SMALL);

    setupCategories();
    
    auto* toolHBox = new wxBoxSizer(wxHORIZONTAL);
    toolHBox->Add(createLeftToolBar(), 1, wxEXPAND);
    toolHBox->Add(createRightToolBar(), 0, wxEXPAND);

    _mainWidget->GetSizer()->Add(toolHBox, 0, wxEXPAND);
    _mainWidget->GetSizer()->Add(_listView, 1, wxEXPAND);
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
    _listView->ClearAll();

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

            _listView->InsertItem(_listView->GetItemCount(), displayName, category.iconIndex);
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

    // We need to create the liststore and widgets before attaching ourselves
    // to the material manager as observer, as the attach() call below
    // will invoke a realise() callback, which triggers a population
    construct();

    // The startup event will add this page to the group dialog tab
    GlobalMainFrame().signal_MainFrameConstructed().connect(
        sigc::mem_fun(*this, &FavouritesBrowser::onMainFrameConstructed)
    );

    reloadFavourites();
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
    reloadFavourites(); // TODO: lazy
}

module::StaticModule<FavouritesBrowser> favouritesBrowserModule;

}
