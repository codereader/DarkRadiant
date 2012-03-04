#pragma once

#include "MaterialsList.h"

#include "modelskin.h"
#include "iradiant.h"
#include "iuimanager.h"

#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/preview/ModelPreview.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/KeyValueTable.h"

#include <string>
#include <gtkmm/treestore.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treeselection.h>

namespace Gtk
{
	class TreeView;
	class Expander;
	class CheckButton;
	class VBox;
}

namespace ui
{

/**
 * Data structure containing the model, the skin name and the options to be returned from
 * the Model Selector.
 */
struct ModelSelectorResult
{
	// Model and skin strings
	std::string model;
	std::string skin;

	// options
	bool createClip;

	// Constructor
	ModelSelectorResult(const std::string& m, const std::string& s, const bool clip)
	: model(m), skin(s), createClip(clip) {}
};

class ModelSelector;
typedef boost::shared_ptr<ModelSelector> ModelSelectorPtr;

/// Dialog for browsing and selecting a model and/or skin
class ModelSelector :
	public gtkutil::BlockingTransientWindow,
    private gtkutil::GladeWidgetHolder
{
public:
	// Treemodel definition
	struct TreeColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		TreeColumns()
		{
			add(filename);
			add(vfspath);
			add(skin);
			add(icon);
			add(isFolder);
		}

		Gtk::TreeModelColumn<std::string> filename;		// e.g. "chair1.lwo"
		Gtk::TreeModelColumn<std::string> vfspath;		// e.g. "models/darkmod/props/chair1.lwo"
		Gtk::TreeModelColumn<std::string> skin;			// e.g. "chair1_brown_wood", or "" for no skin
		Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon; // icon to display
		Gtk::TreeModelColumn<bool> isFolder;				// whether this is a folder
	};

private:
	TreeColumns _columns;

	// Model preview widget
    gtkutil::ModelPreviewPtr _modelPreview;

	// Tree store containing model names (one with and one without skins)
	Glib::RefPtr<Gtk::TreeStore> _treeStore;
	Glib::RefPtr<Gtk::TreeStore> _treeStoreWithSkins;

    // Main tree view with model hierarchy
	Gtk::TreeView* _treeView;

	// Currently-selected row in the tree store
	Glib::RefPtr<Gtk::TreeSelection> _selection;

    // Key/value table for model information
    gtkutil::KeyValueTable _infoTable;

    // Materials list table
    MaterialsList _materialsList;

	// The window position tracker
	gtkutil::WindowPosition _position;

	// Last selected model, which will be returned by showAndBlock() once the
	// recursive main loop exits.
	std::string _lastModel;
	std::string _lastSkin;

	// TRUE if the treeview has been populated
	bool _populated;

    // Whether to show advanced options panel
    bool _showOptions;

private:
	// Private constructor, creates widgets
	ModelSelector();

	// Home of the static instance
	static ModelSelector& Instance();

	// This is where the static shared_ptr of the singleton instance is held.
	static ModelSelectorPtr& InstancePtr();

	// Show the dialog, called internally by chooseModel(). Return the selected model path
	ModelSelectorResult showAndBlock(const std::string& curModel,
                                     bool showOptions,
                                     bool showSkins);

	// Helper functions to configure GUI components
    void setupAdvancedPanel();
	void setupTreeView();

	// Populate the tree view with models
	void populateModels();

	// Initialise the GL widget, to avoid doing this every frame
	void initialisePreview();

	// Update the info table with information from the currently-selected model, and
	// update the displayed model.
	void showInfoForSelectedModel();

	// Return the value from the selected column, or an empty string if nothing selected
	std::string getSelectedValue(int col);

	// gtkmm callbacks
	void callbackOK();
	void callbackCancel();

protected:
	// Override TransientWindow::_onDeleteEvent
	void _onDeleteEvent();

	// Override BlockingTransientWindow::_postShow()
	void _postShow();

public:
	/**
	 * Display the Model Selector instance, constructing it on first use, and
	 * return the VFS path of the model selected by the user. When the
	 * ModelSelector is displayed it will enter a recursive gtk_main loop,
	 * blocking execution of the calling function until destroyed.
	 *
	 * @curModel: the name of the currently selected model the tree will browse to
	 *            Leave this empty to leave the treeview focus where it was when
	 *            the dialog was closed.
	 *
	 * @showOptions: whether to show the advanced options tab.
	 */
	static ModelSelectorResult chooseModel(
			const std::string& curModel = "", bool showOptions = true, bool showSkins = true);

	// greebo: Lets the modelselector repopulate its treeview next time the dialog is shown.
	static void refresh();

	void onRadiantShutdown();
};

}
