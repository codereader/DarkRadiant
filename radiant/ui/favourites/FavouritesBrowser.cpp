#include "FavouritesBrowser.h"

#include "ifavourites.h"
#include "iuimanager.h"
#include "module/StaticModule.h"
#include <wx/artprov.h>
#include <wx/toolbar.h>

namespace ui
{

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

    // Add the toolbar
    auto* toolbar = new wxToolBar(_mainWidget, wxID_ANY);
    toolbar->SetToolBitmapSize(wxSize(24, 24));

    for (auto& category : _categories)
    {
        category.checkButton = toolbar->AddCheckTool(wxID_ANY, category.displayName,
            wxArtProvider::GetBitmap(prefix + category.iconName, wxART_TOOLBAR));

        category.checkButton->SetShortHelp(category.displayName);

        toolbar->Bind(wxEVT_TOOL, &FavouritesBrowser::onCategoryToggled, this, category.checkButton->GetId());
    }

    toolbar->Realize();

    _mainWidget->GetSizer()->Add(toolbar, 0, wxEXPAND);
    _mainWidget->GetSizer()->Add(_listView, 1, wxEXPAND);
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
            _listView->InsertItem(_listView->GetItemCount(), fav, category.iconIndex);
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
        _dependencies.insert(MODULE_FAVOURITES_MANAGER);
    }

    return _dependencies;
}

void FavouritesBrowser::initialiseModule(const IApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;

    //GlobalCommandSystem().addCommand("ToggleFavouritesBrowser", sigc::mem_fun(this, &FavouritesBrowser::togglePage));

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
}

void FavouritesBrowser::onMainFrameConstructed()
{
    // Add the Media Browser page
    auto page = std::make_shared<IGroupDialog::Page>();

    page->name = "favourites";
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

module::StaticModule<FavouritesBrowser> favouritesBrowserModule;

}
