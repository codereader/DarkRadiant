#include "MediaBrowser.h"

#include <wx/display.h>

#include "ideclmanager.h"
#include "imap.h"
#include "ishaders.h"
#include "ishaderclipboard.h"

#include "wxutil/dataview/ResourceTreeViewToolbar.h"

#include <wx/sizer.h>
#include <wx/app.h>
#include <wx/popupwin.h>

#include "util/ScopedBoolLock.h"
#include "ui/texturebrowser/TextureBrowserManager.h"
#include "ui/common/TexturePreviewCombo.h"

#include "FocusMaterialRequest.h"
#include "ui/UserInterfaceModule.h"
#include "ui/texturebrowser/TextureDirectoryBrowser.h"
#include "wxutil/MultiMonitor.h"
#include "wxutil/TransientPopupWindow.h"

namespace ui
{

MediaBrowser::MediaBrowser(wxWindow* parent) :
    DockablePanel(parent),
	_treeView(nullptr),
	_preview(nullptr),
	_blockShaderClipboardUpdates(false),
    _reloadTreeOnIdle(false),
    _showThumbnailBrowserOnIdle(false)
{
    construct();
    queueTreeReload();

    connectListeners();
}

MediaBrowser::~MediaBrowser()
{
    if (panelIsActive())
    {
        disconnectListeners();
    }

    closePopup();
    _treeView = nullptr;
}

void MediaBrowser::closePopup()
{
    _showThumbnailBrowserOnIdle = false;

    if (!_browserPopup.get()) return;

    if (!wxTheApp->IsScheduledForDestruction(_browserPopup.get()))
    {
        _browserPopup->Dismiss();
    }

    _browserPopup.Release();
}

void MediaBrowser::onPanelActivated()
{
    // Request an idle callback now to process pending updates
    // that might have piled up while we've been inactive
    requestIdleCallback();
}

void MediaBrowser::onPanelDeactivated()
{
    closePopup();
}

void MediaBrowser::connectListeners()
{
    // These listeners remain connected even if the mediabrowser is inactive

    // Attach to the DeclarationManager to get notified on unrealise/realise
    // events, in which case we're reloading the media tree
    _materialDefsLoaded = GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::Material)
        .connect(sigc::mem_fun(*this, &MediaBrowser::onMaterialDefsLoaded));

    _materialDefsUnloaded = GlobalDeclarationManager().signal_DeclsReloading(decl::Type::Material)
        .connect(sigc::mem_fun(*this, &MediaBrowser::onMaterialDefsUnloaded));

    // The tree view will be re-populated once the first map loaded signal is fired
    _mapLoadedConn = GlobalMapModule().signal_mapEvent().connect(
        sigc::mem_fun(this, &MediaBrowser::onMapEvent)
    );

    _shaderClipboardConn = GlobalShaderClipboard().signal_sourceChanged().connect(
        sigc::mem_fun(this, &MediaBrowser::onShaderClipboardSourceChanged)
    );

    _focusMaterialHandler = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::FocusMaterialRequest,
        radiant::TypeListener<FocusMaterialRequest>(
            sigc::mem_fun(this, &MediaBrowser::focusMaterial)));
}

void MediaBrowser::disconnectListeners()
{
    _mapLoadedConn.disconnect();
    _materialDefsLoaded.disconnect();
    _materialDefsUnloaded.disconnect();
    _shaderClipboardConn.disconnect();
    GlobalRadiantCore().getMessageBus().removeListener(_focusMaterialHandler);
}

void MediaBrowser::onIdle()
{
    if (_reloadTreeOnIdle)
    {
        _reloadTreeOnIdle = false;
        _treeView->Populate();
    }

    if (!_queuedSelection.empty())
    {
        util::ScopedBoolLock lock(_blockShaderClipboardUpdates);

        setSelection(_queuedSelection);
        _queuedSelection.clear();
    }

    if (_showThumbnailBrowserOnIdle)
    {
        _showThumbnailBrowserOnIdle = false;

        if (_treeView->IsDirectorySelected())
        {
            // When a directory is selected, open the popup
            auto popup = findOrCreateBrowserPopup();

            auto browser = static_cast<TextureDirectoryBrowser*>(popup->FindWindow("TextureDirectoryBrowser"));
            assert(browser);

            // Set the (new) texture path and show the popup
            browser->setTexturePath(_treeView->GetSelectedTextureFolderName());
            
            popup->Popup();
        }
    }
}

wxutil::TransientPopupWindow* MediaBrowser::findOrCreateBrowserPopup()
{
    // Check if we have any previous popup that is not scheduled to be destroyed
    if (_browserPopup.get() && !wxTheApp->IsScheduledForDestruction(_browserPopup.get()))
    {
        // We can re-use this one
        return _browserPopup.get();
    }

    // Create a new one
    auto popup = new wxutil::TransientPopupWindow(this);

    // Remember this popup
    _browserPopup = popup;

    auto browser = new TextureDirectoryBrowser(popup, _treeView->GetSelectedTextureFolderName());
    browser->SetName("TextureDirectoryBrowser");
    popup->GetSizer()->Add(browser, 1, wxEXPAND | wxALL, 3);

    // Size reaching from the upper edge of the mediabrowser to the bottom of the screen (minus a few pixels)
    auto rectScreen = wxutil::MultiMonitor::getMonitorForWindow(this);
    int verticalOffset = -(GetScreenPosition().y - rectScreen.GetY()) / 2;
    wxSize size(630, rectScreen.GetY() + rectScreen.GetHeight() - GetScreenPosition().y - verticalOffset - 40);

    popup->PositionNextTo(this, verticalOffset, size);
    popup->Layout();

    return popup;
}

void MediaBrowser::queueTreeReload()
{
    _reloadTreeOnIdle = true;

    // Queue an update if the panel is visible
    if (panelIsActive())
    {
        requestIdleCallback();
    }
}

void MediaBrowser::construct()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	_treeView = new MediaBrowserTreeView(this);
    auto* toolbar = new wxutil::ResourceTreeViewToolbar(this, _treeView);

	GetSizer()->Add(toolbar, 0, wxALIGN_LEFT | wxEXPAND | wxALL, 6);
	GetSizer()->Add(_treeView, 1, wxEXPAND);

	// Connect up the selection changed callback
	_treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &MediaBrowser::_onTreeViewSelectionChanged, this);

	// Add the info pane
	_preview = new TexturePreviewCombo(this);
	GetSizer()->Add(_preview, 0, wxEXPAND);

	// When destroying the main widget clear out the held references.
	// The dying populator thread might have posted a finished message which
	// runs into problems when the _treeView is still valid
	Bind(wxEVT_DESTROY, [&](wxWindowDestroyEvent& ev)
	{
		// In wxGTK the destroy event might bubble from a child window
		// like the search popup, so check the event object
		if (ev.GetEventObject() == this)
		{
            disconnectListeners();
            _treeView = nullptr;
		}
		ev.Skip();
	});
}

void MediaBrowser::onMapEvent(IMap::MapEvent ev)
{
    // Re-populate the tree after a map has been loaded, this way
    // we can list all the missing textures that get auto-generated
    // during map realisation (#5475)
    if (ev == IMap::MapEvent::MapLoaded)
    {
        queueTreeReload();
    }
}

std::string MediaBrowser::getSelection()
{
    return _treeView->GetSelectedDeclName();
}

void MediaBrowser::setSelection(const std::string& selection)
{
    _treeView->SetSelectedDeclName(selection);
}

void MediaBrowser::onMaterialDefsLoaded()
{
    queueTreeReload();
}

void MediaBrowser::onMaterialDefsUnloaded()
{
    queueTreeReload();
}

void MediaBrowser::_onTreeViewSelectionChanged(wxDataViewEvent& ev)
{
    // Update the preview if a texture is selected
    if (!_treeView->IsDirectorySelected())
    {
        _preview->SetPreviewDeclName(getSelection());
        // Close any pending popups
        closePopup();
    }
    else
    {
        _preview->ClearPreview();

        _showThumbnailBrowserOnIdle = true;
        requestIdleCallback();
    }

    sendSelectionToShaderClipboard();
}

void MediaBrowser::sendSelectionToShaderClipboard()
{
    if (_blockShaderClipboardUpdates) return;

    util::ScopedBoolLock lock(_blockShaderClipboardUpdates);

    if (!_treeView->IsDirectorySelected())
    {
        GlobalShaderClipboard().setSourceShader(getSelection());
    }
    else
    {
        // Nothing selected, clear the clipboard
        GlobalShaderClipboard().clear();
    }
}

void MediaBrowser::queueSelection(const std::string& material)
{
    _queuedSelection = material;

    if (panelIsActive())
    {
        requestIdleCallback();
    }
}

void MediaBrowser::onShaderClipboardSourceChanged()
{
    if (_blockShaderClipboardUpdates) return;

    // Queue this selection for later
    queueSelection(GlobalShaderClipboard().getShaderName());
}

void MediaBrowser::focusMaterial(FocusMaterialRequest& request)
{
    queueSelection(request.getRequestedMaterial());
}

} // namespace
