#include "ifavourites.h"
#include "imainframe.h"
#include "icommandsystem.h"
#include "igroupdialog.h"
#include "itextstream.h"

#include "module/StaticModule.h"
#include "FavouritesBrowser.h"

namespace ui
{

namespace
{
    const char* const TAB_NAME = "favourites";
}

class FavouritesUserInterfaceModule :
    public RegisterableModule
{
public:
    // RegisterableModule
    const std::string& getName() const override
    {
        static std::string _name("FavouritesUserInterface");
        return _name;
    }

    const StringSet& getDependencies() const override
    {
        static StringSet _dependencies;

        if (_dependencies.empty())
        {
            _dependencies.insert(MODULE_FAVOURITES_MANAGER);
            _dependencies.insert(MODULE_UIMANAGER);
            _dependencies.insert(MODULE_COMMANDSYSTEM);
            _dependencies.insert(MODULE_MAINFRAME);
        }

        return _dependencies;
    }

    void initialiseModule(const IApplicationContext& ctx) override
    {
        rMessage() << getName() << "::initialiseModule called." << std::endl;

        GlobalCommandSystem().addCommand("ToggleFavouritesBrowser",
            sigc::mem_fun(this, &FavouritesUserInterfaceModule::togglePage));

        // Subscribe to get notified as soon as Radiant is fully initialised
        GlobalMainFrame().signal_MainFrameConstructed().connect(
            sigc::mem_fun(this, &FavouritesUserInterfaceModule::onMainFrameConstructed)
        );
    }

private:
    void togglePage(const cmd::ArgumentList& args)
    {
        GlobalGroupDialog().togglePage(TAB_NAME);
    }

    void onMainFrameConstructed()
    {
        // Add the tab to the groupdialog
        auto page = std::make_shared<IGroupDialog::Page>();

        page->name = TAB_NAME;
        page->windowLabel = _("Favourites");
        page->page = new FavouritesBrowser(GlobalMainFrame().getWxTopLevelWindow());
        page->tabIcon = "favourite.png";
        page->tabLabel = _("Favourites");
        page->position = IGroupDialog::Page::Position::Favourites;

        GlobalGroupDialog().addPage(page);
    }
};

module::StaticModule<FavouritesUserInterfaceModule> favouritesUserInterfaceModule;

}
