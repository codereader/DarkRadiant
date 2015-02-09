#include "TextureBrowserManager.h"
#include "TextureBrowser.h"

namespace ui
{

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

TextureBrowserManager& TextureBrowserManager::Instance()
{
    static ui::TextureBrowserManager instance;
    return instance;
}

} // namespace
