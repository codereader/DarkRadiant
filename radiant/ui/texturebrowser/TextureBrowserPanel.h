#pragma once

#include <sigc++/connection.h>

#include "wxutil/DockablePanel.h"

namespace ui
{

class MapTextureBrowser;
class TextureBrowserManager;

class TextureBrowserPanel :
    public wxutil::DockablePanel,
    public sigc::trackable
{
private:
    sigc::connection _favouritesChangedHandler;

    MapTextureBrowser* _thumbnailBrowser;

public:
    TextureBrowserPanel(wxWindow* parent);
    ~TextureBrowserPanel() override;

    void queueUpdate();

    // Returns the currently selected shader
    const std::string& getSelectedShader() const;

    // Sets the currently selected shader to <newShader> and
    // refocuses the texturebrowser to that shader.
    void setSelectedShader(const std::string& newShader);

protected:
    void onPanelActivated() override;
    void onPanelDeactivated() override;

private:
    void connectListeners();
    void disconnectListeners();

    void onFavouritesChanged();
};

}

// Accessor method to the singleton instance
ui::TextureBrowserManager& GlobalTextureBrowser();
