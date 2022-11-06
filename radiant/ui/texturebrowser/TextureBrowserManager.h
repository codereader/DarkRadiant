#pragma once

#include <set>
#include <sigc++/connection.h>
#include "imodule.h"

namespace ui
{

class TextureBrowserPanel;

constexpr const char* const RKEY_TEXTURES_HIDE_UNUSED = "user/ui/textures/browser/hideUnused";
constexpr const char* const RKEY_TEXTURES_SHOW_FAVOURITES_ONLY = "user/ui/textures/browser/showFavouritesOnly";
constexpr const char* const RKEY_TEXTURES_SHOW_OTHER_MATERIALS = "user/ui/textures/browser/showOtherMaterials";
constexpr const char* const RKEY_TEXTURES_SHOW_NAMES = "user/ui/textures/browser/showNames";
constexpr const char* const RKEY_TEXTURE_UNIFORM_SIZE = "user/ui/textures/browser/uniformSize";
constexpr const char* const RKEY_TEXTURE_USE_UNIFORM_SCALE = "user/ui/textures/browser/useUniformScale";
constexpr const char* const RKEY_TEXTURE_SCALE = "user/ui/textures/browser/textureScale";
constexpr const char* const RKEY_TEXTURE_SHOW_SCROLLBAR = "user/ui/textures/browser/showScrollBar";
constexpr const char* const RKEY_TEXTURE_MOUSE_WHEEL_INCR = "user/ui/textures/browser/mouseWheelIncrement";
constexpr const char* const RKEY_TEXTURE_SHOW_FILTER = "user/ui/textures/browser/showFilter";
constexpr const char* const RKEY_TEXTURE_CONTEXTMENU_EPSILON = "user/ui/textures/browser/contextMenuMouseEpsilon";
constexpr const char* const RKEY_TEXTURE_MAX_NAME_LENGTH = "user/ui/textures/browser/maxShadernameLength";

class TextureBrowserManager :
    public RegisterableModule
{
private:
    std::set<TextureBrowserPanel*> _browsers;
    sigc::connection _shaderClipboardConn;

public:
    TextureBrowserManager();

    // Use the currently selected shader (kind of legacy method)
    std::string getSelectedShader();
    void setSelectedShader(const std::string& shader);

    void registerTextureBrowser(TextureBrowserPanel* browser);
    void unregisterTextureBrowser(TextureBrowserPanel* browser);

    // Sends an queueUpdate() call to all registered browsers
    void updateAllWindows();

    static TextureBrowserManager& Instance();

    // RegisterableModule
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

private:
    void registerPreferencePage();
    void onShaderClipboardSourceChanged();
};

} // namespace

