#include "TextureBrowser.h"

#include "TextureBrowserManager.h"
#include "ifavourites.h"
#include "MaterialThumbnailBrowser.h"

namespace ui
{

TextureBrowser::TextureBrowser(wxWindow* parent) :
    DockablePanel(parent),
    _thumbnailBrowser(nullptr)
{
    _thumbnailBrowser = new MaterialThumbnailBrowser(this);

    SetSizer(new wxBoxSizer(wxVERTICAL));
    GetSizer()->Add(_thumbnailBrowser, 1, wxEXPAND);

    GlobalTextureBrowser().registerTextureBrowser(this);
}

TextureBrowser::~TextureBrowser()
{
    if (panelIsActive())
    {
        disconnectListeners();
    }
    GlobalTextureBrowser().unregisterTextureBrowser(this);
}

const std::string& TextureBrowser::getSelectedShader() const
{
    return _thumbnailBrowser->getSelectedShader();
}

void TextureBrowser::setSelectedShader(const std::string& newShader)
{
    _thumbnailBrowser->setSelectedShader(newShader);
}

void TextureBrowser::onPanelActivated()
{
    connectListeners();
    queueUpdate();
}

void TextureBrowser::onPanelDeactivated()
{
    disconnectListeners();
}

void TextureBrowser::connectListeners()
{
    _favouritesChangedHandler = GlobalFavouritesManager().getSignalForType(decl::getTypeName(decl::Type::Material))
        .connect(sigc::mem_fun(this, &TextureBrowser::onFavouritesChanged));
}

void TextureBrowser::disconnectListeners()
{
    _favouritesChangedHandler.disconnect();
}

void TextureBrowser::onFavouritesChanged()
{
    queueUpdate();
}

void TextureBrowser::queueUpdate()
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
