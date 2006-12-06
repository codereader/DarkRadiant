#ifndef LIGHTTEXTURESELECTOR_H_
#define LIGHTTEXTURESELECTOR_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkliststore.h>

#include <string>

/* FORWARD DECLS */

class IShader;

namespace ui
{

/** A widget that allows the selection of a light texture. The widget contains
 * three elements - a tree view displaying available light textures as
 * identified by prefixes in the .game file, an OpenGL widget displaying a 
 * preview of the currently-selected texture, and a table containing certain
 * information about the texture.
 * 
 * This widget populates its list of textures automatically, and offers a method
 * that allows calling code to retrieve the user's selection.
 */

class LightTextureSelector
{
	// Main widget container
	GtkWidget* _widget;
	
	// Selection for the tree view
	GtkTreeSelection* _selection;
	
	// GL preview widget
	GtkWidget* _glWidget;
	
	// List store for info table
	GtkListStore* _infoStore;
	
private:

	// Create GUI elements
	GtkWidget* createTreeView();
	GtkWidget* createPreview();
	
	// Update the info in the table
	void updateInfoTable();
	
	// Get the selected IShader
	IShader* getSelectedShader();
	
	/* GTK CALLBACKS */
	static void _onExpose(GtkWidget*, GdkEventExpose*, LightTextureSelector*);
	static void _onSelChange(GtkWidget*, LightTextureSelector*);
	
public:

	/** Constructor.
	 */
	LightTextureSelector();
	
	/** Operator cast to GtkWidget*, for packing into parent widget.
	 */
	operator GtkWidget* () {
		return _widget;
	}
	
	/** Return the texture selected by the user, or an empty string if there
	 * was no selection.
	 */
	std::string getSelection();
};

}

#endif /*LIGHTTEXTURESELECTOR_H_*/
