#include "MediaBrowser.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "ipreferencesystem.h"
#include "ishaders.h"
#include "ishaderclipboard.h"
#include "ieventmanager.h"
#include "ifavourites.h"

#include "wxutil/MultiMonitor.h"
#include "wxutil/dataview/ResourceTreeViewToolbar.h"

#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/frame.h>

#include <iostream>
#include <map>

#include "registry/registry.h"
#include "string/string.h"
#include "util/ScopedBoolLock.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "ui/common/TexturePreviewCombo.h"

#include "debugging/ScopedDebugTimer.h"
#include "module/StaticModule.h"

#include <functional>
#include "string/predicate.h"

namespace ui
{

// Constructor
MediaBrowser::MediaBrowser() : 
	_tempParent(nullptr),
	_mainWidget(nullptr),
	_treeView(nullptr),
	_preview(nullptr),
	_blockShaderClipboardUpdates(false)
{}

void MediaBrowser::construct()
{
	if (_mainWidget != nullptr)
	{
		return;
	}

	_tempParent = new wxFrame(nullptr, wxID_ANY, "");
	_tempParent->Hide();

	_mainWidget = new wxPanel(_tempParent, wxID_ANY); 
	_mainWidget->SetSizer(new wxBoxSizer(wxVERTICAL));

	_treeView = new MediaBrowserTreeView(_mainWidget);
    auto* toolbar = new wxutil::ResourceTreeViewToolbar(_mainWidget, _treeView);

	_mainWidget->GetSizer()->Add(toolbar, 0, wxALIGN_LEFT | wxEXPAND | wxALL, 6);
	_mainWidget->GetSizer()->Add(_treeView, 1, wxEXPAND);

	// Connect up the selection changed callback
	_treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &MediaBrowser::_onTreeViewSelectionChanged, this);

	// Add the info pane
	_preview = new TexturePreviewCombo(_mainWidget);
	_mainWidget->GetSizer()->Add(_preview, 0, wxEXPAND);

	// When destroying the main widget clear out the held references.
	// The dying populator thread might have posted a finished message which 
	// runs into problems when the _treeView is still valid
	_mainWidget->Bind(wxEVT_DESTROY, [&](wxWindowDestroyEvent& ev)
	{
		// In wxGTK the destroy event might bubble from a child window 
		// like the search popup, so check the event object
		if (ev.GetEventObject() == _mainWidget)
		{
			_treeView = nullptr;
			_shaderClipboardConn.disconnect();
			_materialDefsLoaded.disconnect();
			_materialDefsUnloaded.disconnect();
		}
		ev.Skip();
	});
}

void MediaBrowser::onMainFrameConstructed()
{
	// Add the Media Browser page
	IGroupDialog::PagePtr mediaBrowserPage(new IGroupDialog::Page);

	mediaBrowserPage->name = getGroupDialogTabName();
	mediaBrowserPage->windowLabel = _("Media");
	mediaBrowserPage->page = _mainWidget;
	mediaBrowserPage->tabIcon = "folder16.png";
	mediaBrowserPage->tabLabel = _("Media");
	mediaBrowserPage->position = IGroupDialog::Page::Position::MediaBrowser;

	GlobalGroupDialog().addPage(mediaBrowserPage);

	if (_tempParent != nullptr)
	{
		_tempParent->Destroy();
		_tempParent = nullptr;
	}
}

std::string MediaBrowser::getSelection()
{
    return _treeView->GetSelectedFullname();
}

// Set the selection in the treeview
void MediaBrowser::setSelection(const std::string& selection)
{
    _treeView->SetSelectedFullname(selection);
}

void MediaBrowser::onMaterialDefsLoaded()
{
    _treeView->Populate();
}

void MediaBrowser::onMaterialDefsUnloaded()
{
    _treeView->Clear();
}

void MediaBrowser::_onTreeViewSelectionChanged(wxDataViewEvent& ev)
{
    util::ScopedBoolLock lock(_blockShaderClipboardUpdates);

    // Update the preview if a texture is selected
    if (!_treeView->IsDirectorySelected())
    {
        _preview->SetTexture(getSelection());
        GlobalShaderClipboard().setSourceShader(getSelection());
    }
    else
    {
        _preview->SetTexture("");
        // Nothing selected, clear the clipboard
        GlobalShaderClipboard().clear();
    }
}

void MediaBrowser::togglePage(const cmd::ArgumentList& args)
{
	GlobalGroupDialog().togglePage(getGroupDialogTabName());
}

const std::string& MediaBrowser::getName() const
{
	static std::string _name("MediaBrowser");
	return _name;
}

const StringSet& MediaBrowser::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_SHADERSYSTEM);
		_dependencies.insert(MODULE_UIMANAGER);
		_dependencies.insert(MODULE_SHADERCLIPBOARD);
		_dependencies.insert(MODULE_MAINFRAME);
		_dependencies.insert(MODULE_FAVOURITES_MANAGER);
	}

	return _dependencies;
}

void MediaBrowser::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	GlobalCommandSystem().addCommand("ToggleMediaBrowser", sigc::mem_fun(this, &MediaBrowser::togglePage));

	// We need to create the liststore and widgets before attaching ourselves
	// to the material manager as observer, as the attach() call below
	// will invoke a realise() callback, which triggers a population
	construct();

	// The startup event will add this page to the group dialog tab
	GlobalMainFrame().signal_MainFrameConstructed().connect(
		sigc::mem_fun(*this, &MediaBrowser::onMainFrameConstructed)
	);

	// Attach to the MaterialManager to get notified on unrealise/realise
	// events, in which case we're reloading the media tree
	_materialDefsLoaded = GlobalMaterialManager().signal_DefsLoaded().connect(
		sigc::mem_fun(*this, &MediaBrowser::onMaterialDefsLoaded)
	);

	_materialDefsUnloaded = GlobalMaterialManager().signal_DefsUnloaded().connect(
		sigc::mem_fun(*this, &MediaBrowser::onMaterialDefsUnloaded)
	);

	// Start loading materials
	_treeView->Populate();

	_shaderClipboardConn = GlobalShaderClipboard().signal_sourceChanged().connect(
		sigc::mem_fun(this, &MediaBrowser::onShaderClipboardSourceChanged)
	);
}

void MediaBrowser::shutdownModule()
{
	_shaderClipboardConn.disconnect();
	_materialDefsLoaded.disconnect();
	_materialDefsUnloaded.disconnect();
}

void MediaBrowser::onShaderClipboardSourceChanged()
{
	if (_blockShaderClipboardUpdates)
	{
		return;
	}

	setSelection(GlobalShaderClipboard().getShaderName());
}

// Static module
module::StaticModule<MediaBrowser> mediaBrowserModule;

} // namespace
