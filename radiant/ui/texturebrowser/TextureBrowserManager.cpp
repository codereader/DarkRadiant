#include "TextureBrowserManager.h"
#include "TextureBrowserPanel.h"

#include <list>
#include <sigc++/functors/mem_fun.h>
#include "i18n.h"
#include "ui/ieventmanager.h"
#include "ui/iuserinterface.h"
#include "ishaderclipboard.h"
#include "icommandsystem.h"
#include "ipreferencesystem.h"
#include "module/StaticModule.h"

namespace ui
{

namespace
{
    constexpr const char* const MODULE_TEXTURE_BROWSER_MANAGER = "TextureBrowserManager";
}

class TextureBrowserControl :
    public IUserControl
{
public:
    std::string getControlName() override
    {
        return UserControl::TextureBrowser;
    }

    std::string getDisplayName() override
    {
        return _("Textures");
    }

    std::string getIcon() override
    {
        return "icon_texture.png";
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new TextureBrowserPanel(parent);
    }
};

TextureBrowserManager::TextureBrowserManager()
{}

std::string TextureBrowserManager::getSelectedShader()
{
    if (_browsers.empty()) return "";

    return (*_browsers.begin())->getSelectedShader();
}

void TextureBrowserManager::setSelectedShader(const std::string& shader)
{
    for (auto browser : _browsers)
    {
        browser->setSelectedShader(shader);
    }
}

void TextureBrowserManager::registerTextureBrowser(TextureBrowserPanel* browser)
{
    _browsers.insert(browser);
}

void TextureBrowserManager::unregisterTextureBrowser(TextureBrowserPanel* browser)
{
    _browsers.erase(browser);
}

void TextureBrowserManager::updateAllWindows()
{
    for (auto browser : _browsers)
    {
        browser->queueUpdate();
    }
}

void TextureBrowserManager::registerPreferencePage()
{
    // Add a page to the given group
	IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Texture Browser"));

    std::list<std::string> textureScaleList
    {
        "5%", "10%", "25%", "50%", "100%", "200%",
    };

    page.appendCombo(_("Texture Thumbnail Scale"), RKEY_TEXTURE_SCALE, textureScaleList);

    page.appendCheckBox(_("Scale Thumbnails to Uniform size"), RKEY_TEXTURE_USE_UNIFORM_SCALE);
    page.appendEntry(_("Uniform Thumbnail Size (pixels)"), RKEY_TEXTURE_UNIFORM_SIZE);
    page.appendCheckBox(_("Texture scrollbar"), RKEY_TEXTURE_SHOW_SCROLLBAR);
    page.appendEntry(_("Mousewheel Increment"), RKEY_TEXTURE_MOUSE_WHEEL_INCR);
    page.appendSpinner(_("Max shadername length"), RKEY_TEXTURE_MAX_NAME_LENGTH, 4, 100, 1);

    page.appendCheckBox(_("Show Texture Filter"), RKEY_TEXTURE_SHOW_FILTER);
    page.appendCheckBox(_("Show \"Other Materials\""), RKEY_TEXTURES_SHOW_OTHER_MATERIALS);
}

const std::string& TextureBrowserManager::getName() const
{
    static std::string _name(MODULE_TEXTURE_BROWSER_MANAGER);
    return _name;
}

const StringSet& TextureBrowserManager::getDependencies() const
{
    static StringSet _dependencies;

    if (_dependencies.empty())
    {
        _dependencies.insert(MODULE_XMLREGISTRY);
        _dependencies.insert(MODULE_EVENTMANAGER);
        _dependencies.insert(MODULE_COMMANDSYSTEM);
        _dependencies.insert(MODULE_SHADERCLIPBOARD);
        _dependencies.insert(MODULE_USERINTERFACE);
    }

    return _dependencies;
}

void TextureBrowserManager::initialiseModule(const IApplicationContext& ctx)
{
    GlobalEventManager().addRegistryToggle("TextureBrowserToggleUniformScale", RKEY_TEXTURE_USE_UNIFORM_SCALE);
    GlobalEventManager().addRegistryToggle("TextureBrowserHideUnused", RKEY_TEXTURES_HIDE_UNUSED);
    GlobalEventManager().addRegistryToggle("TextureBrowserShowFavouritesOnly", RKEY_TEXTURES_SHOW_FAVOURITES_ONLY);
    GlobalEventManager().addRegistryToggle("TextureBrowserShowNames", RKEY_TEXTURES_SHOW_NAMES);

    registerPreferencePage();

    _shaderClipboardConn = GlobalShaderClipboard().signal_sourceChanged().connect(
        sigc::mem_fun(this, &TextureBrowserManager::onShaderClipboardSourceChanged)
    );

    // Register the texture browser
    GlobalUserInterface().registerControl(std::make_shared<TextureBrowserControl>());

    GlobalMainFrame().signal_MainFrameConstructed().connect([this]
    {
        GlobalMainFrame().addControl(UserControl::TextureBrowser, IMainFrame::ControlSettings
        {
            IMainFrame::Location::PropertyPanel,
            true
        });
    });
}

void TextureBrowserManager::shutdownModule()
{
    GlobalUserInterface().unregisterControl(UserControl::TextureBrowser);
    _shaderClipboardConn.disconnect();
}

void TextureBrowserManager::onShaderClipboardSourceChanged()
{
    // Get the shaderclipboard shader and try to highlight it
    // if the shader name is empty, it will unfocus the selection
    setSelectedShader(GlobalShaderClipboard().getShaderName());
}

// Define the static module
module::StaticModuleRegistration<TextureBrowserManager> texBrowserManagerModule;

TextureBrowserManager& TextureBrowserManager::Instance()
{
    return *std::static_pointer_cast<TextureBrowserManager>(
        module::GlobalModuleRegistry().getModule(MODULE_TEXTURE_BROWSER_MANAGER));
}

} // namespace
