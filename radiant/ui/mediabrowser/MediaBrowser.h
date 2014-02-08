#pragma once

#include "iradiant.h"
#include "imodule.h"
#include "icommandsystem.h"
#include "ui/common/TexturePreviewCombo.h"

#include "gtkutil/TreeModel.h"
#include "gtkutil/menu/PopupMenu.h"

#include <gtkmm/treestore.h>
#include <gtkmm/treeselection.h>

#include <wx/event.h>

class wxWindow;
class wxTreeCtrl;
class wxFrame;
class wxDataViewCtrl;
class wxDataViewTreeStore;

namespace Gtk
{
	class TreeView;
	class Widget;
	class VBox;
}

namespace ui
{

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
			displayName(add(wxutil::TreeModel::Column::String)),
			fullName(add(wxutil::TreeModel::Column::String)),
			isFolder(add(wxutil::TreeModel::Column::Bool)),
			isOtherMaterialsFolder(add(wxutil::TreeModel::Column::Bool))
		{}

		wxutil::TreeModel::Column displayName;
		wxutil::TreeModel::Column fullName;
		//wxutil::TreeModel::Column icon;
		wxutil::TreeModel::Column isFolder;
		wxutil::TreeModel::Column isOtherMaterialsFolder;
	};

		// Treemodel definition
	struct GtkTreeColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		GtkTreeColumns()
		{
			add(displayName);
			add(fullName);
			add(icon);
			add(isFolder);
			add(isOtherMaterialsFolder);
		}

		Gtk::TreeModelColumn<std::string> displayName; // std::string is sorting faster
		Gtk::TreeModelColumn<std::string> fullName;
		Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;
		Gtk::TreeModelColumn<bool> isFolder;
		Gtk::TreeModelColumn<bool> isOtherMaterialsFolder;
	};


	class PopulatorFinishedEvent; // wxEvent type

private:
	wxFrame* _tempParent;

	wxWindow* _mainWidget;

	wxDataViewCtrl* _wxTreeView;
	GtkTreeColumns _columns;
	TreeColumns _wxColumns;
	wxutil::TreeModel* _wxTreeStore;

	// Main widget
	boost::shared_ptr<Gtk::VBox> _widget;

	// Main tree store, view and selection
	Glib::RefPtr<Gtk::TreeStore> _treeStore;
	Gtk::TreeView* _treeView;
	Glib::RefPtr<Gtk::TreeSelection> _selection;

	// Populates the Media Browser in its own thread
    class Populator;
    boost::shared_ptr<Populator> _populator;

	// Context menu
	gtkutil::PopupMenu _popupMenu;

	// Texture preview combo (GL widget and info table)
	TexturePreviewCombo* _preview;

	// false, if the tree is not yet initialised.
	bool _isPopulated;

private:

	/* gtkutil::PopupMenu callbacks */
	bool _testSingleTexSel();
	bool _testLoadInTexView();
	void _onApplyToSel();
	void _onLoadInTexView();
	void _onShowShaderDefinition();

	/* GTK CALLBACKS */

	bool _onExpose(GdkEventExpose*);
	void _onSelectionChanged();

	/* Tree selection query functions */

	bool isDirectorySelected(); // is a directory selected
	std::string getSelectedName(); // return name of selection

	// Populates the treeview
	void populate();

	void getTreeStoreFromLoader();
	void onTreeStorePopulationFinished(PopulatorFinishedEvent& ev);

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
	/*Gtk::Widget* getWidget()
	{
		assert(_widget != NULL);
		_widget->show_all();
		return _widget.get();
	}*/

	wxWindow* getWidget()
	{
		return _mainWidget;
	}

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

	/**
	 * greebo: Custom tree sort function to list folders before textures
	 */
	int treeViewSortFunc(const Gtk::TreeModel::iterator& a, const Gtk::TreeModel::iterator& b);
};

}
