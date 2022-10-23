#include "MediaBrowser.h"

#include "ideclmanager.h"
#include "imap.h"
#include "ishaders.h"
#include "ishaderclipboard.h"
#include "ui/iuserinterface.h"

#include "wxutil/dataview/ResourceTreeViewToolbar.h"

#include <wx/sizer.h>

#include "util/ScopedBoolLock.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "ui/common/TexturePreviewCombo.h"

#include "FocusMaterialRequest.h"
#include "ui/UserInterfaceModule.h"

namespace ui
{

MediaBrowser::MediaBrowser(wxWindow* parent) :
    DockablePanel(parent),
	_treeView(nullptr),
	_preview(nullptr),
	_blockShaderClipboardUpdates(false),
    _reloadTreeOnIdle(false)
{
    construct();
    queueTreeReload();

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
}

MediaBrowser::~MediaBrowser()
{
    destroy();
}

void MediaBrowser::destroy()
{
    disconnectListeners();
    _mapLoadedConn.disconnect();
    _materialDefsLoaded.disconnect();
    _materialDefsUnloaded.disconnect();
    _shaderClipboardConn.disconnect();
    _treeView = nullptr;
}

void MediaBrowser::onPanelActivated()
{
    connectListeners();

    // Request an idle callback now to process pending updates
    requestIdleCallback();
}

void MediaBrowser::onPanelDeactivated()
{
    disconnectListeners();
}

void MediaBrowser::connectListeners()
{
    _focusMaterialHandler = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::FocusMaterialRequest,
        radiant::TypeListener<ui::FocusMaterialRequest>(
            sigc::mem_fun(this, &MediaBrowser::focusMaterial)));
}

void MediaBrowser::disconnectListeners()
{
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
        setSelection(_queuedSelection);
        _queuedSelection.clear();
    }
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
            destroy();
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
    util::ScopedBoolLock lock(_blockShaderClipboardUpdates);

    // Update the preview if a texture is selected
    if (!_treeView->IsDirectorySelected())
    {
        _preview->SetPreviewDeclName(getSelection());
        GlobalShaderClipboard().setSourceShader(getSelection());
    }
    else
    {
        _preview->ClearPreview();
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
    GlobalGroupDialog().setPage(UserControl::MediaBrowser);
}

} // namespace
