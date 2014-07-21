#pragma once

#include "iradiant.h"
#include "imodule.h"
#include "icommandsystem.h"

#include "wxutil/TreeView.h"
#include "wxutil/menu/PopupMenu.h"

#include <wx/event.h>

class wxWindow;
class wxTreeCtrl;
class wxFrame;
class wxDataViewTreeStore;
class wxTreeEvent;

namespace ui
{

class TexturePreviewCombo;

class MediaBrowser;
typedef boost::shared_ptr<MediaBrowser> MediaBrowserPtr;

/**
 * \brief Media Browser page of the group dialog.
 *
 * This page allows browsing of individual textures by name and loading them
 * into the texture window or applying directly to map geometry.
 */
class MediaBrowser : 
	public sigc::trackable,
	public wxEvtHandler,
	public ModuleObserver // to monitor the MaterialManager module
{
public:
	struct TreeColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		TreeColumns() :
			iconAndName(add(wxutil::TreeModel::Column::IconText)),
			leafName(add(wxutil::TreeModel::Column::String)),
			fullName(add(wxutil::TreeModel::Column::String)),
			isFolder(add(wxutil::TreeModel::Column::Boolean)),
			isOtherMaterialsFolder(add(wxutil::TreeModel::Column::Boolean))
		{}

		wxutil::TreeModel::Column iconAndName;
		wxutil::TreeModel::Column leafName; // no parent folders
		wxutil::TreeModel::Column fullName;
		wxutil::TreeModel::Column isFolder;
		wxutil::TreeModel::Column isOtherMaterialsFolder;
	};

	class PopulatorFinishedEvent; // wxEvent type

private:
	wxFrame* _tempParent;

	wxWindow* _mainWidget;

	wxutil::TreeView* _treeView;
	TreeColumns _columns;
	wxutil::TreeModel* _treeStore;

	// Populates the Media Browser in its own thread
    class Populator;
    std::unique_ptr<Populator> _populator;

	// Context menu
	wxutil::PopupMenuPtr _popupMenu;

	// Texture preview combo (GL widget and info table)
	TexturePreviewCombo* _preview;

	// false, if the tree is not yet initialised.
	bool _isPopulated;

private:
	void construct();

	/* wxutil::PopupMenu callbacks */
	bool _testSingleTexSel();
	bool _testLoadInTexView();
	void _onApplyToSel();
	void _onLoadInTexView();
	void _onShowShaderDefinition();

	/* wx CALLBACKS */
	void _onExpose(wxPaintEvent& ev);
	void _onSelectionChanged(wxTreeEvent& ev);
	void _onContextMenu(wxDataViewEvent& ev);

	void handleSelectionChange();

	/* Tree selection query functions */
	bool isDirectorySelected(); // is a directory selected
	std::string getSelectedName(); // return name of selection

	// Populates the treeview
	void populate();

	void onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev);

	/** Return the singleton instance.
	 */
	static MediaBrowserPtr& getInstancePtr();

public:

	/** Return the singleton instance.
	 */
	static MediaBrowser& getInstance();

	/** Return the main widget for packing into
	 * the groupdialog or other parent container.
	 */
	wxWindow* getWidget();

	/** Constructor creates GTK widgets.
	 */
	MediaBrowser();

	/** Set the given path as the current selection, highlighting it
	 * in the tree view.
	 *
	 * @param selection
	 * The fullname of the item to select, or the empty string if there
	 * should be no selection.
	 */
	void setSelection(const std::string& selection);

	/** greebo: Rebuilds the media tree from scratch (call this after
	 * 			a "RefreshShaders" command.
	 */
	void reloadMedia();

	/**
	 * greebo: Handles the media tree preload
	 */
	static void init();

	/**
	 * greebo: Registers the preference page and the commands
	 */
	static void registerCommandsAndPreferences();

	// ModuleObserver implementation, these are called when the MaterialManager
	// is emitting realise signals
	void unrealise();
	void realise();

	// Radiant Event Listener
	void onRadiantShutdown();

	/**
	 * greebo: Static command target for toggling the mediabrowser tab in the groupdialog.
	 */
	static void toggle(const cmd::ArgumentList& args);
};

}
