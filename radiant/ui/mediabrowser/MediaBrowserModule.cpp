#include "MediaBrowser.h"

#include "module/StaticModule.h"
#include "icommandsystem.h"
#include "ui/iuserinterface.h"

namespace ui
{

class MediaBrowserControl :
    public IUserControl
{
public:
    std::string getControlName() override
    {
        return UserControl::MediaBrowser;
    }

    std::string getDisplayName() override
    {
        return _("Media");
    }

    std::string getIcon() override
    {
        return "folder16.png";
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new MediaBrowser(parent);
    }
};

class MediaBrowserModule :
    public RegisterableModule
{
public:
    const std::string& getName() const override
    {
        static std::string _name("MediaBrowser");
        return _name;
    }

    const StringSet& getDependencies() const override
    {
        static StringSet _dependencies
        {
            MODULE_COMMANDSYSTEM,
            MODULE_USERINTERFACE,
        };

        return _dependencies;
    }

    void initialiseModule(const IApplicationContext& ctx) override
    {
        GlobalCommandSystem().addCommand("ToggleMediaBrowser", sigc::mem_fun(this, &MediaBrowserModule::togglePage));

        GlobalUserInterface().registerControl(std::make_shared<MediaBrowserControl>());
    }

    void shutdownModule() override
    {
        GlobalUserInterface().unregisterControl(UserControl::MediaBrowser);
    }

private:
    /**
    * greebo: Command target for toggling the mediabrowser tab in the groupdialog.
    */
    void togglePage(const cmd::ArgumentList& args)
    {
        GlobalGroupDialog().togglePage(UserControl::MediaBrowser);
    }
};

// Static module
module::StaticModuleRegistration<MediaBrowserModule> _mediaBrowserModule;

}
