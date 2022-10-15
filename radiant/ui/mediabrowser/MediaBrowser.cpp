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
	wxPanel(parent),
	_treeView(nullptr),
	_preview(nullptr),
	_blockShaderClipboardUpdates(false)
{
    construct();

    // Attach to the DeclarationManager to get notified on unrealise/realise
    // events, in which case we're reloading the media tree
    _materialDefsLoaded = GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::Material)
        .connect(sigc::mem_fun(*this, &MediaBrowser::onMaterialDefsLoaded));

    _materialDefsUnloaded = GlobalDeclarationManager().signal_DeclsReloading(decl::Type::Material)
        .connect(sigc::mem_fun(*this, &MediaBrowser::onMaterialDefsUnloaded));

    _shaderClipboardConn = GlobalShaderClipboard().signal_sourceChanged().connect(
        sigc::mem_fun(this, &MediaBrowser::onShaderClipboardSourceChanged)
    );

    // The tree view will be re-populated once the first map loaded signal is fired
    _mapLoadedConn = GlobalMapModule().signal_mapEvent().connect(
        sigc::mem_fun(this, &MediaBrowser::onMapEvent)
    );

    _focusMaterialHandler = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::FocusMaterialRequest,
        radiant::TypeListener<ui::FocusMaterialRequest>(
            sigc::mem_fun(this, &MediaBrowser::focusMaterial)));

    _treeView->Populate();
}

MediaBrowser::~MediaBrowser()
{
    GlobalRadiantCore().getMessageBus().removeListener(_focusMaterialHandler);
    _mapLoadedConn.disconnect();
    _shaderClipboardConn.disconnect();
    _materialDefsLoaded.disconnect();
    _materialDefsUnloaded.disconnect();
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
			_treeView = nullptr;
			_shaderClipboardConn.disconnect();
			_materialDefsLoaded.disconnect();
			_materialDefsUnloaded.disconnect();
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
        _treeView->Populate();
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
    GlobalUserInterface().dispatch([this]() { _treeView->Populate(); });
}

void MediaBrowser::onMaterialDefsUnloaded()
{
    GlobalUserInterface().dispatch([this]() { _treeView->Clear(); });
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

void MediaBrowser::onShaderClipboardSourceChanged()
{
	if (_blockShaderClipboardUpdates)
	{
		return;
	}

	setSelection(GlobalShaderClipboard().getShaderName());
}

void MediaBrowser::focusMaterial(FocusMaterialRequest& request)
{
    setSelection(request.getRequestedMaterial());
    GlobalGroupDialog().setPage(UserControl::MediaBrowser);
}

} // namespace
