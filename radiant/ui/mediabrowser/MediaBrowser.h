#pragma once

#include <sigc++/connection.h>

#include "imap.h"

#include "MediaBrowserTreeView.h"

#include "wxutil/DockablePanel.h"
#include "wxutil/event/SingleIdleCallback.h"

namespace wxutil { class TransientPopupWindow; }

class wxWindow;
class wxTreeCtrl;
class wxFrame;
class wxDataViewTreeStore;
class wxTreeEvent;
class wxRadioButton;

namespace ui
{

class FocusMaterialRequest;
class TexturePreviewCombo;

/**
 * \brief Media Browser control
 *
 * This control allows browsing of individual textures by name and loading them
 * into the texture window or applying directly to map geometry.
 */
class MediaBrowser : 
	public wxutil::DockablePanel,
    public wxutil::SingleIdleCallback
{
private:
	MediaBrowserTreeView* _treeView;

	// Texture preview combo (GL widget and info table)
	TexturePreviewCombo* _preview;

	sigc::connection _materialDefsLoaded;
	sigc::connection _materialDefsUnloaded;
	sigc::connection _shaderClipboardConn;
	sigc::connection _mapLoadedConn;

	bool _blockShaderClipboardUpdates;
	bool _reloadTreeOnIdle;
    bool _showThumbnailBrowserOnIdle;
    std::string _queuedSelection;

    std::size_t _focusMaterialHandler;

    wxWeakRef<wxutil::TransientPopupWindow> _browserPopup;

public:
	MediaBrowser(wxWindow* parent);
    ~MediaBrowser() override;

	// Returns the currently selected item, or an empty string if nothing is selected
	std::string getSelection();

	/** Set the given path as the current selection, highlighting it
	 * in the tree view.
	 *
	 * @param selection
	 * The fullname of the item to select, or the empty string if there
	 * should be no selection.
	 */
	void setSelection(const std::string& selection);

protected:
    void onIdle() override;
    void onPanelActivated() override;
    void onPanelDeactivated() override;

private:
    void construct();
    void _onTreeViewSelectionChanged(wxDataViewEvent& ev);

    void queueTreeReload();
    void queueSelection(const std::string& material);

    void connectListeners();
    void disconnectListeners();

	// These are called when the MaterialManager is loading/unloading the defs
	void onMaterialDefsUnloaded();
	void onMaterialDefsLoaded();

	void onMapEvent(IMap::MapEvent ev);
    void focusMaterial(FocusMaterialRequest& request);

	void onShaderClipboardSourceChanged();
	void sendSelectionToShaderClipboard();
    wxutil::TransientPopupWindow* findOrCreateBrowserPopup();
    void closePopup();
};

}
