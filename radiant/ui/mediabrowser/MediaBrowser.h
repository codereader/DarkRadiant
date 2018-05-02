#pragma once

#include <sigc++/connection.h>

#include "imediabrowser.h"
#include "iradiant.h"
#include "imodule.h"
#include "icommandsystem.h"

#include "wxutil/TreeView.h"
#include "wxutil/menu/PopupMenu.h"
#include "wxutil/TreeModelFilter.h"

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
	public IMediaBrowser,
	public wxEvtHandler
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
			isOtherMaterialsFolder(add(wxutil::TreeModel::Column::Boolean)),
			isFavourite(add(wxutil::TreeModel::Column::Boolean))
		{}

		wxutil::TreeModel::Column iconAndName;
		wxutil::TreeModel::Column leafName; // no parent folders
		wxutil::TreeModel::Column fullName;
		wxutil::TreeModel::Column isFolder;
		wxutil::TreeModel::Column isOtherMaterialsFolder;
		wxutil::TreeModel::Column isFavourite;
	};

	class PopulatorFinishedEvent; // wxEvent type
	class Favourites;

private:
	wxFrame* _tempParent;

	wxWindow* _mainWidget;

	wxRadioButton* _showAll;
	wxRadioButton* _showFavourites;

	wxutil::TreeView* _treeView;
	TreeColumns _columns;
	wxutil::TreeModel::Ptr _treeStore;
	wxutil::TreeModelFilter::Ptr _treeModelFilter;
	wxDataViewItem _emptyFavouritesLabel;

	enum class TreeMode
	{
		ShowAll,
		ShowFavourites,
	};
	TreeMode _mode;

	// Populates the Media Browser in its own thread
    class Populator;
    std::unique_ptr<Populator> _populator;

	std::unique_ptr<Favourites> _favourites;

	// Context menu
	wxutil::PopupMenuPtr _popupMenu;

	// Texture preview combo (GL widget and info table)
	TexturePreviewCombo* _preview;

	// false, if the tree is not yet initialised.
	bool _isPopulated;

	sigc::connection _materialDefsLoaded;
	sigc::connection _materialDefsUnloaded;

private:
	void construct();

	/* wxutil::PopupMenu callbacks */
	bool _testSingleTexSel();
	bool _testLoadInTexView();
	void _onApplyToSel();
	void _onLoadInTexView();
	void _onShowShaderDefinition();
    void _onSelectItems(bool select);
	bool _testAddToFavourites();
	bool _testRemoveFromFavourites();
	void _onSetFavourite(bool isFavourite);

	// Sets favourite status on this item and all below
	void setFavouriteRecursively(wxutil::TreeModel::Row& row, bool isFavourite);

	/* wx CALLBACKS */
	void _onExpose(wxPaintEvent& ev);
	void _onSelectionChanged(wxTreeEvent& ev);
	void _onContextMenu(wxDataViewEvent& ev);

	void handleSelectionChange();
	void handleTreeModeChanged();

	/* Tree selection query functions */
	bool isDirectorySelected(); // is a directory selected
	bool isFavouriteSelected(); // is a favourite selected

	// Populates the treeview
	void populate();

	void onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev);

	// Evaulation function for item visibility
	bool treeModelFilterFunc(wxutil::TreeModel::Row& row);

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
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	// These are called when the MaterialManager is loading/unloading the defs
	void onMaterialDefsUnloaded();
	void onMaterialDefsLoaded();

	// Radiant Event Listener
	void onRadiantStartup();

	/**
	* greebo: Command target for toggling the mediabrowser tab in the groupdialog.
	*/
	void togglePage(const cmd::ArgumentList& args);

	void setupTreeViewAndFilter();
};

}
