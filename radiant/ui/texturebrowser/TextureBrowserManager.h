#pragma once

#include <set>

namespace ui
{

class TextureBrowser;

class TextureBrowserManager
{
private:
    std::set<TextureBrowser*> _browsers;

public:
    TextureBrowserManager();

    // Use the currently selected shader (kind of legacy method)
    std::string getSelectedShader();
    void setSelectedShader(const std::string& shader);

    void registerTextureBrowser(TextureBrowser* browser);
    void unregisterTextureBrowser(TextureBrowser* browser);

    static TextureBrowserManager& Instance();
};

} // namespace

