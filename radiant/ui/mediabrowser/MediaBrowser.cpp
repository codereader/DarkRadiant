#include "MediaBrowser.h"

#include <wx/display.h>

#include "ideclmanager.h"
#include "imap.h"
#include "ishaders.h"
#include "icommandsystem.h"
#include "ishaderclipboard.h"

#include "wxutil/dataview/ResourceTreeViewToolbar.h"
#include "wxutil/Bitmap.h"

#include <wx/sizer.h>
#include <wx/tglbtn.h>
#include <wx/checkbox.h>
#include <wx/app.h>
#include <wx/popupwin.h>

#include "util/ScopedBoolLock.h"
#include "registry/registry.h"
#include "ui/texturebrowser/TextureBrowserManager.h"
#include "ui/common/TexturePreviewCombo.h"
#include "ui/materials/MaterialThumbnailBrowser.h"

#include "FocusMaterialRequest.h"
#include "ui/UserInterfaceModule.h"
#include "ui/texturebrowser/TextureDirectoryBrowser.h"
#include "wxutil/MultiMonitor.h"
#include "wxutil/TransientPopupWindow.h"

namespace ui
{

namespace
{
    constexpr const char* const RKEY_MEDIABROWSER_VIEW_MODE = "user/ui/mediaBrowser/viewMode";
}

MediaBrowser::MediaBrowser(wxWindow* parent) :
    DockablePanel(parent),
	_treeView(nullptr),
	_preview(nullptr),
	_thumbnailBrowser(nullptr),
	_viewToggleBtn(nullptr),
	_uniformScaleCheckbox(nullptr),
	_showingThumbnails(false),
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
    GlobalRegistry().set(RKEY_MEDIABROWSER_VIEW_MODE,
                         _showingThumbnails ? "thumbnails" : "tree");

    _thumbnailSelectionConn.disconnect();
    _thumbnailActivatedConn.disconnect();
    _filterTextChangedConn.disconnect();

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

    if (_thumbnailBrowser)
    {
        _thumbnailBrowser->queueUpdate();
    }
}

void MediaBrowser::construct()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	_treeView = new MediaBrowserTreeView(this);
    auto* toolbar = new wxutil::ResourceTreeViewToolbar(this, _treeView);

	createViewToggleButton(toolbar);
	createUniformScaleCheckbox(toolbar);

	GetSizer()->Add(toolbar, 0, wxALIGN_LEFT | wxEXPAND | wxALL, 6);
	GetSizer()->Add(_treeView, 1, wxEXPAND);

	// Connect up the selection changed callback
	_treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &MediaBrowser::_onTreeViewSelectionChanged, this);

	_filterTextChangedConn = toolbar->signal_filterTextChanged().connect(
		sigc::mem_fun(this, &MediaBrowser::onFilterTextChanged));

	// Add the info pane
	_preview = new TexturePreviewCombo(this);
	GetSizer()->Add(_preview, 0, wxEXPAND);

	// Restore saved view mode
	std::string savedMode = GlobalRegistry().get(RKEY_MEDIABROWSER_VIEW_MODE);
	if (savedMode == "thumbnails")
	{
		_showingThumbnails = true;
		_viewToggleBtn->SetValue(true);
		switchView(true);
	}

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
    if (_showingThumbnails && _thumbnailBrowser)
    {
        return _thumbnailBrowser->getSelectedShader();
    }
    return _treeView->GetSelectedDeclName();
}

void MediaBrowser::setSelection(const std::string& selection)
{
    _treeView->SetSelectedDeclName(selection);

    if (_thumbnailBrowser)
    {
        _thumbnailBrowser->setSelectedShader(selection);
    }
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

        if (!_showingThumbnails && _thumbnailBrowser)
        {
            _thumbnailBrowser->setSelectedShader(_treeView->GetSelectedDeclName());
        }
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

    std::string selection = getSelection();

    if (!selection.empty())
    {
        GlobalShaderClipboard().setSourceShader(selection);
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

void MediaBrowser::createThumbnailBrowser()
{
    _thumbnailBrowser = new MaterialThumbnailBrowser(this, MaterialThumbnailBrowser::TextureFilter::All);
    _thumbnailBrowser->Hide();

    GetSizer()->Insert(2, _thumbnailBrowser, 1, wxEXPAND);

    _thumbnailSelectionConn = _thumbnailBrowser->signal_selectionChanged().connect(
        sigc::mem_fun(this, &MediaBrowser::onThumbnailSelectionChanged));

    _thumbnailActivatedConn = _thumbnailBrowser->signal_itemActivated().connect(
        sigc::mem_fun(this, &MediaBrowser::onThumbnailItemActivated));
}

MaterialThumbnailBrowser* MediaBrowser::getOrCreateThumbnailBrowser()
{
    if (!_thumbnailBrowser)
    {
        createThumbnailBrowser();
    }

    return _thumbnailBrowser;
}

void MediaBrowser::createViewToggleButton(wxutil::ResourceTreeViewToolbar* toolbar)
{
    _viewToggleBtn = new wxBitmapToggleButton(toolbar, wxID_ANY,
        wxutil::GetLocalBitmap("bgimage16.png"));
    _viewToggleBtn->SetToolTip(_("Toggle between tree view and thumbnail grid view"));
    toolbar->GetRightSizer()->Add(_viewToggleBtn, wxSizerFlags().Border(wxLEFT, 6));
    _viewToggleBtn->Bind(wxEVT_TOGGLEBUTTON, &MediaBrowser::onViewToggle, this);
}

void MediaBrowser::createUniformScaleCheckbox(wxutil::ResourceTreeViewToolbar* toolbar)
{
    _uniformScaleCheckbox = new wxCheckBox(toolbar, wxID_ANY, _("Grid"));
    _uniformScaleCheckbox->SetValue(registry::getValue<bool>(RKEY_TEXTURE_USE_UNIFORM_SCALE));
    _uniformScaleCheckbox->Hide();
    toolbar->GetLeftSizer()->Add(_uniformScaleCheckbox, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
    _uniformScaleCheckbox->Bind(wxEVT_CHECKBOX, &MediaBrowser::onUniformScaleToggle, this);
}

void MediaBrowser::switchView(bool showThumbnails)
{
    if (showThumbnails)
    {
        auto* browser = getOrCreateThumbnailBrowser();
        browser->setSelectedShader(_treeView->GetSelectedDeclName());
        _treeView->Hide();
        browser->Show();
        browser->queueUpdate();
        _uniformScaleCheckbox->Show();
    }
    else
    {
        if (_thumbnailBrowser)
        {
            std::string selectedShader = _thumbnailBrowser->getSelectedShader();
            if (!selectedShader.empty())
            {
                setSelection(selectedShader);
            }
            _thumbnailBrowser->Hide();
        }
        _treeView->Show();
        _uniformScaleCheckbox->Hide();
    }

    _showingThumbnails = showThumbnails;
    Layout();
}

void MediaBrowser::onViewToggle(wxCommandEvent& ev)
{
    switchView(ev.IsChecked());
}

void MediaBrowser::onUniformScaleToggle(wxCommandEvent& ev)
{
    registry::setValue(RKEY_TEXTURE_USE_UNIFORM_SCALE, ev.IsChecked());
}

void MediaBrowser::onThumbnailSelectionChanged()
{
    std::string selected = _thumbnailBrowser->getSelectedShader();
    _preview->SetPreviewDeclName(selected);
    sendSelectionToShaderClipboard();
}

void MediaBrowser::onThumbnailItemActivated()
{
    std::string selection = _thumbnailBrowser->getSelectedShader();

    if (!selection.empty())
    {
        GlobalCommandSystem().executeCommand("SetShaderOnSelection", selection);
    }
}

void MediaBrowser::onFilterTextChanged(const std::string& filterText)
{
    if (_thumbnailBrowser)
    {
        _thumbnailBrowser->setExternalFilterText(filterText);
    }
}

} // namespace
