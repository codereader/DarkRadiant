#ifndef MODELSELECTOR_H_
#define MODELSELECTOR_H_

#include "modelskin.h"
#include "iradiant.h"
#include "iuimanager.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/WindowPosition.h"

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

/** Singleton class encapsulating the Model Selector dialog and methods required to display the
 * dialog and retrieve the selected model.
 */

class ModelSelector :
	public gtkutil::BlockingTransientWindow,
	public RadiantEventListener
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

	struct InfoStoreColumns : 
		public Gtk::TreeModel::ColumnRecord
	{
		InfoStoreColumns() { add(attribute); add(value); }

		Gtk::TreeModelColumn<Glib::ustring> attribute;
		Gtk::TreeModelColumn<Glib::ustring> value;
	};

private:
	Gtk::VBox* _vbox; // main vbox

	TreeColumns _columns;

	// Model preview widget
	IModelPreviewPtr _modelPreview;
	
	// Tree store containing model names (one with and one without skins)
	Glib::RefPtr<Gtk::TreeStore> _treeStore;
	Glib::RefPtr<Gtk::TreeStore> _treeStoreWithSkins;

	Gtk::TreeView* _treeView;
	
	// Currently-selected row in the tree store
	Glib::RefPtr<Gtk::TreeSelection> _selection;
	
	// List store to contain attributes and values for the selected model
	InfoStoreColumns _infoStoreColumns;
	Glib::RefPtr<Gtk::ListStore> _infoStore;

	// options widgets
	Gtk::Expander* _advancedOptions;
	Gtk::CheckButton* _clipCheckButton;

	// The window position tracker
	gtkutil::WindowPosition _position;
	
	// Last selected model, which will be returned by showAndBlock() once the
	// recursive main loop exits.
	std::string _lastModel;
	std::string _lastSkin;
	
	// TRUE if the treeview has been populated
	bool _populated;

	bool _showOptions;
	
private:
	// Private constructor, creates widgets
	ModelSelector();
	
	// Home of the static instance
	static ModelSelector& Instance();

	// This is where the static shared_ptr of the singleton instance is held.
	static ModelSelectorPtr& InstancePtr();

	// Show the dialog, called internally by chooseModel(). Return the selected model path
	ModelSelectorResult showAndBlock(const std::string& curModel, bool showOptions, bool showSkins);
	
	// Helper functions to create GUI components
	Gtk::Widget& createTreeView();
	Gtk::Widget& createButtons();
	Gtk::Widget& createAdvancedButtons();
	Gtk::Widget& createInfoPanel();
	
	// Populate the tree view with models
	void populateModels();
	
	// Initialise the GL widget, to avoid doing this every frame
	void initialisePreview();
	
	// Update the info table with information from the currently-selected model, and
	// update the displayed model.
	void updateSelected();
	
	// Return the value from the selected column, or an empty string if nothing selected
	std::string getSelectedValue(int col);
	
	// gtkmm callbacks	
	void callbackSelChanged();
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

	// RadiantEventListener implementation
	void onRadiantShutdown();
};

}

#endif /*MODELSELECTOR_H_*/
