#ifndef MODELSELECTOR_H_
#define MODELSELECTOR_H_

#include "modelskin.h"
#include "ui/common/ModelPreview.h"

#include <gtk/gtkwidget.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkliststore.h>

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
 * Data structure containing both a model and a skin name, to be returned from
 * the Model Selector.
 */
struct ModelAndSkin {
	// Model and skin strings
	std::string model;
	std::string skin;

	// Constructor
	ModelAndSkin(const std::string& m, const std::string& s)
	: model(m), skin(s) {}
};

/** Singleton class encapsulating the Model Selector dialog and methods required to display the
 * dialog and retrieve the selected model.
 */

class ModelSelector
: private GlobalModelSkinCacheModuleRef // instantiate the model skin cache first
{
private:

	// Main dialog widget
	GtkWidget* _widget;

	// Model preview widget
	ModelPreview _modelPreview;
	
	// Tree store containing model names
	GtkTreeStore* _treeStore;
	
	// Currently-selected row in the tree store
	GtkTreeSelection* _selection;
	
	// List store to contain attributes and values for the selected model
	GtkListStore* _infoStore;
	
	// Last selected model, which will be returned by showAndBlock() once the
	// recursive main loop exits.
	std::string _lastModel;
	std::string _lastSkin;
	
private:
	
	// Private constructor, creates GTK widgets
	ModelSelector();

	// Show the dialog, called internally by chooseModel(). Return the selected model path
	ModelAndSkin showAndBlock();
	
	// Helper functions to create GUI components
	GtkWidget* createTreeView();
	GtkWidget* createButtons();
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

	/** Display the Model Selector instance, constructing it on first use, and return
	 * the VFS path of the model selected by the user. When the ModelSelector is displayed
	 * it will enter a recursive gtk_main loop, blocking execution of the calling
	 * function until destroyed.
	 */
	 
	static ModelAndSkin chooseModel();
	
};

}

#endif /*MODELSELECTOR_H_*/
