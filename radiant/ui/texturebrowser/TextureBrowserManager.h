#pragma once

#include <set>
#include "imodule.h"
#include "icommandsystem.h"

namespace ui
{

class TextureBrowser;

class TextureBrowserManager :
    public RegisterableModule
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

    // Sends an queueUpdate() call to all registered browsers
    void updateAllWindows();

    static TextureBrowserManager& Instance();

    // RegisterableModule
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const ApplicationContext& ctx) override;

private:
    static void toggleGroupDialogTexturesTab(const cmd::ArgumentList& args);
    void registerPreferencePage();
};

} // namespace

