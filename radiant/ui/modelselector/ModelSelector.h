#ifndef MODELSELECTOR_H_
#define MODELSELECTOR_H_

#include <gtk/gtk.h>
#include <GL/glew.h>

#include "modelskin.h"
#include "imodel.h"

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
	
	// GL preview widget
	GtkWidget* _glWidget;
	
	// Tree store containing model names
	GtkTreeStore* _treeStore;
	
	// Currently-selected row in the tree store
	GtkTreeSelection* _selection;
	
	// List store to contain attributes and values for the selected model
	GtkListStore* _infoStore;
	
	// Last selected model, which will be returned by showAndBlock() once the
	// recursive main loop exits.
	std::string _lastModel;
	
	// Current distance between camera and preview
	GLfloat _camDist;
	
	// Current model to display
	model::IModelPtr _model;
	
private:
	
	// Private constructor, creates GTK widgets
	ModelSelector();

	// Show the dialog, called internally by chooseModel(). Return the selected model path
	std::string showAndBlock();
	
	// Helper functions to create GUI components
	GtkWidget* createTreeView();
	GtkWidget* createButtons();
	GtkWidget* createPreviewAndInfoPanel();
	GtkWidget* createGLWidget();
	
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
	static void callbackGLDraw(GtkWidget*, GdkEventExpose*, ModelSelector*);
	static void callbackGLMotion(GtkWidget*, GdkEventMotion*, ModelSelector*);
	static void callbackGLScroll(GtkWidget*, GdkEventScroll*, ModelSelector*);
	
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
