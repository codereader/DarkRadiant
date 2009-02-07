#ifndef MODELSELECTOR_H_
#define MODELSELECTOR_H_

#include "modelskin.h"
#include "iradiant.h"
#include "ui/common/ModelPreview.h"

#include <gtk/gtkwidget.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkliststore.h>
#include "gtkutil/WindowPosition.h"

#include <string>

namespace ui
{

/* CONSTANTS */
namespace {
	
	const char* MODELSELECTOR_TITLE = "Choose model";
	const char* MODELS_FOLDER = "models/";

	// Treestore enum
	enum {
		NAME_COLUMN,		// e.g. "chair1.lwo"
		FULLNAME_COLUMN,	// e.g. "models/darkmod/props/chair1.lwo"
		SKIN_COLUMN,		// e.e. "chair1_brown_wood", or "" for no skin
		IMAGE_COLUMN,		// icon to display
		N_COLUMNS
	};
	
}

/** 
 * Data structure containing the model, the skin name and the options to be returned from
 * the Model Selector.
 */
struct ModelSelectorResult {
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
	public RadiantEventListener
{
private:
	// Main dialog widget
	GtkWidget* _widget;

	// Model preview widget
	ModelPreviewPtr _modelPreview;
	
	// Tree store containing model names (one with and one without skins)
	GtkTreeStore* _treeStore;
	GtkTreeStore* _treeStoreWithSkins;

	GtkTreeView* _treeView;
	
	// Currently-selected row in the tree store
	GtkTreeSelection* _selection;
	
	// List store to contain attributes and values for the selected model
	GtkListStore* _infoStore;

	// options widgets
	GtkExpander* _advancedOptions;
	GtkCheckButton* _clipCheckButton;

	// The window position tracker
	gtkutil::WindowPosition _position;
	
	// Last selected model, which will be returned by showAndBlock() once the
	// recursive main loop exits.
	std::string _lastModel;
	std::string _lastSkin;
	
	// TRUE if the treeview has been populated
	bool _populated;
	
private:
	
	// Private constructor, creates GTK widgets
	ModelSelector();
	
	// Home of the static instance
	static ModelSelector& Instance();

	// This is where the static shared_ptr of the singleton instance is held.
	static ModelSelectorPtr& InstancePtr();

	// Show the dialog, called internally by chooseModel(). Return the selected model path
	ModelSelectorResult showAndBlock(const std::string& curModel, bool showOptions, bool showSkins);
	
	// Helper functions to create GUI components
	GtkWidget* createTreeView();
	GtkWidget* createButtons();
	GtkWidget* createAdvancedButtons();
	GtkWidget* createInfoPanel();
	
	// Populate the tree view with models
	void populateModels();
	
	// Initialise the GL widget, to avoid doing this every frame
	void initialisePreview();
	
	// Update the info table with information from the currently-selected model, and
	// update the displayed model.
	void updateSelected();
	
	// Return the value from the selected column, or an empty string if nothing selected
	std::string getSelectedValue(gint col);
	
	/* GTK CALLBACKS */
	
	static void callbackHide(GtkWidget*, GdkEvent*, ModelSelector*);
	static void callbackSelChanged(GtkWidget*, ModelSelector*);
	static void callbackOK(GtkWidget*, ModelSelector*);
	static void callbackCancel(GtkWidget*, ModelSelector*);
	
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
