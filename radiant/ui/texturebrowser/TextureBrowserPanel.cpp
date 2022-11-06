#include "TextureBrowserPanel.h"

#include "TextureBrowserManager.h"
#include "ifavourites.h"
#include "MapTextureBrowser.h"

namespace ui
{

TextureBrowserPanel::TextureBrowserPanel(wxWindow* parent) :
    DockablePanel(parent),
    _thumbnailBrowser(nullptr)
{
    _thumbnailBrowser = new MapTextureBrowser(this);

    SetSizer(new wxBoxSizer(wxVERTICAL));
    GetSizer()->Add(_thumbnailBrowser, 1, wxEXPAND);

    GlobalTextureBrowser().registerTextureBrowser(this);
}

TextureBrowserPanel::~TextureBrowserPanel()
{
    if (panelIsActive())
    {
        disconnectListeners();
    }
    GlobalTextureBrowser().unregisterTextureBrowser(this);
}

const std::string& TextureBrowserPanel::getSelectedShader() const
{
    return _thumbnailBrowser->getSelectedShader();
}

void TextureBrowserPanel::setSelectedShader(const std::string& newShader)
{
    _thumbnailBrowser->setSelectedShader(newShader);
}

void TextureBrowserPanel::onPanelActivated()
{
    connectListeners();
    queueUpdate();
}

void TextureBrowserPanel::onPanelDeactivated()
{
    disconnectListeners();
}

void TextureBrowserPanel::connectListeners()
{
    _favouritesChangedHandler = GlobalFavouritesManager().getSignalForType(decl::getTypeName(decl::Type::Material))
        .connect(sigc::mem_fun(this, &TextureBrowserPanel::onFavouritesChanged));
}

void TextureBrowserPanel::disconnectListeners()
{
    _favouritesChangedHandler.disconnect();
}

void TextureBrowserPanel::onFavouritesChanged()
{
    queueUpdate();
}

void TextureBrowserPanel::queueUpdate()
{
    if (panelIsActive())
    {
        _thumbnailBrowser->queueUpdate();
    }
}


} // namespace

ui::TextureBrowserManager& GlobalTextureBrowser()
{
    return ui::TextureBrowserManager::Instance();
}
