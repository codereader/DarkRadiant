#ifndef MODELSELECTOR_H_
#define MODELSELECTOR_H_

#include <gtk/gtk.h>

#include "modelskin.h"

#include <string>

namespace ui
{

/** Singleton class encapsulating the Model Selector dialog and methods required to display the
 * dialog and retrieve the selected model.
 */

class ModelSelector
: private GlobalModelSkinCacheModuleRef // instantiate the model skin cache first
{
private:

	// Main dialog widget
	GtkWidget* _widget;
	
	// Tree store containing model names
	GtkTreeStore* _treeStore;
	
private:
	
	// Private constructor, creates GTK widgets
	ModelSelector();

	// Show the dialog, called internally by chooseModel(). Return the selected model path
	std::string showAndBlock();
	
	// Helper function to construct the TreeView
	GtkWidget* createTreeView();
	
	/* GTK CALLBACKS */
	
	static void callbackHide(GtkWidget*, GdkEvent*, ModelSelector*);
	
public:

	/** Display the Model Selector instance, constructing it on first use, and return
	 * the VFS path of the model selected by the user. When the ModelSelector is displayed
	 * it will enter a recursive gtk_main loop, blocking execution of the calling
	 * function until destroyed.
	 */
	 
	static std::string chooseModel();
	
};

}

#endif /*MODELSELECTOR_H_*/
