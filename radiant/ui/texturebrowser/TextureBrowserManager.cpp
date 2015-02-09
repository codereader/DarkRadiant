#include "TextureBrowserManager.h"
#include "TextureBrowser.h"

#include <list>
#include "i18n.h"
#include "ieventmanager.h"
#include "icommandsystem.h"
#include "ipreferencesystem.h"
#include "modulesystem/StaticModule.h"

namespace ui
{

namespace
{
    const char* const MODULE_TEXTURE_BROWSER_MANAGER = "TextureBrowserManager";
}

TextureBrowserManager::TextureBrowserManager()
{}

std::string TextureBrowserManager::getSelectedShader()
{
    if (_browsers.empty()) return "";

    return (*_browsers.begin())->getSelectedShader();
}

void TextureBrowserManager::setSelectedShader(const std::string& shader)
{
    for (TextureBrowser* browser : _browsers)
    {
        browser->setSelectedShader(shader);
    }
}

void TextureBrowserManager::registerTextureBrowser(TextureBrowser* browser)
{
    _browsers.insert(browser);
}

void TextureBrowserManager::unregisterTextureBrowser(TextureBrowser* browser)
{
    _browsers.erase(browser);
}

void TextureBrowserManager::registerPreferencePage()
{
    // Add a page to the given group
    PreferencesPagePtr page = GlobalPreferenceSystem().getPage(_("Settings/Texture Browser"));

    // Create the string list containing the texture scalings
    std::list<std::string> textureScaleList;

    textureScaleList.push_back("10%");
    textureScaleList.push_back("25%");
    textureScaleList.push_back("50%");
    textureScaleList.push_back("100%");
    textureScaleList.push_back("200%");

    page->appendCombo(_("Texture Thumbnail Scale"), RKEY_TEXTURE_SCALE, textureScaleList);

    page->appendEntry(_("Uniform texture thumbnail size (pixels)"), RKEY_TEXTURE_UNIFORM_SIZE);
    page->appendCheckBox("", _("Texture scrollbar"), RKEY_TEXTURE_SHOW_SCROLLBAR);
    page->appendEntry(_("Mousewheel Increment"), RKEY_TEXTURE_MOUSE_WHEEL_INCR);
    page->appendSpinner(_("Max shadername length"), RKEY_TEXTURE_MAX_NAME_LENGTH, 4, 100, 1);

    page->appendCheckBox("", _("Show Texture Filter"), RKEY_TEXTURE_SHOW_FILTER);
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
    }

    return _dependencies;
}

void TextureBrowserManager::initialiseModule(const ApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;

    GlobalEventManager().addRegistryToggle("TextureThumbsUniform", RKEY_TEXTURES_CLAMP_TO_UNIFORM_SIZE);
    GlobalEventManager().addRegistryToggle("ShowInUse", RKEY_TEXTURES_HIDE_UNUSED);
    GlobalCommandSystem().addCommand("ViewTextures", TextureBrowser::toggle);
    GlobalEventManager().addCommand("ViewTextures", "ViewTextures");

    registerPreferencePage();
}

// Define the static module
module::StaticModule<TextureBrowserManager> texBrowserManagerModule;

TextureBrowserManager& TextureBrowserManager::Instance()
{
    return *texBrowserManagerModule.getModule();
}

} // namespace
