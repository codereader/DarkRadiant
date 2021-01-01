#pragma once

#include <sigc++/connection.h>

#include "imediabrowser.h"
#include "iradiant.h"
#include "imodule.h"
#include "icommandsystem.h"

#include "MediaBrowserTreeView.h"

#include <wx/event.h>

class wxWindow;
class wxTreeCtrl;
class wxFrame;
class wxDataViewTreeStore;
class wxTreeEvent;
class wxRadioButton;

namespace ui
{

class TexturePreviewCombo;

/**
 * \brief Media Browser page of the group dialog.
 *
 * This page allows browsing of individual textures by name and loading them
 * into the texture window or applying directly to map geometry.
 */
class MediaBrowser : 
	public wxEvtHandler,
	public IMediaBrowser
{
private:
	wxFrame* _tempParent;

	wxWindow* _mainWidget;

	wxRadioButton* _showAll;
	wxRadioButton* _showFavourites;

	MediaBrowserTreeView* _treeView;

	// Texture preview combo (GL widget and info table)
	TexturePreviewCombo* _preview;

	sigc::connection _materialDefsLoaded;
	sigc::connection _materialDefsUnloaded;
	sigc::connection _shaderClipboardConn;

	bool _blockShaderClipboardUpdates;

private:
	void construct();
	void _onTreeViewSelectionChanged(wxDataViewEvent& ev);
	void setTreeModeFromControls();

public:
	/** Constructor creates widgets.
	 */
	MediaBrowser();

	// Returns the currently selected item, or an empty string if nothing is selected
	std::string getSelection() override;

	/** Set the given path as the current selection, highlighting it
	 * in the tree view.
	 *
	 * @param selection
	 * The fullname of the item to select, or the empty string if there
	 * should be no selection.
	 */
	void setSelection(const std::string& selection) override;

	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	// These are called when the MaterialManager is loading/unloading the defs
	void onMaterialDefsUnloaded();
	void onMaterialDefsLoaded();

	void onMainFrameConstructed();

	/**
	* greebo: Command target for toggling the mediabrowser tab in the groupdialog.
	*/
	void togglePage(const cmd::ArgumentList& args);

	void onShaderClipboardSourceChanged();
};

}
