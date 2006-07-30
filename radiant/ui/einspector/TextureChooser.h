#ifndef TEXTURECHOOSER_H_
#define TEXTURECHOOSER_H_

#include <gtk/gtk.h>

#include <string>

namespace ui
{

/* Class encapsulating a Gtk dialog containing a Tree view, which allows
 * a texture to be selected and returned to the TexturePropertyEditor.
 */

class TextureChooser
{
private:

	// The text entry in the PropertyEditor to write the chosen texture
	// into
	GtkWidget* _entry;
	
	// Main dialog widget
	GtkWidget* _widget;	
	
	// Current selection object
	GtkTreeSelection* _selection;
	
	// The preview GtkGLExt widget
	GtkWidget* _glWidget;
	
	// List store containing the shader info for display in the table
	GtkListStore* _infoStore;
	
	// The prefixes we are interested in (from the PropertyEditor's option
	// string
	std::string _prefixes;
	
private:

	// Widget construction helpers
	GtkWidget* createTreeView();
	GtkWidget* createButtons();
	GtkWidget* createPreview();
	
	// Helper function to get selected texture name as a string
	const char* getSelectedName();
	std::string getSelectedImageMap();
	
	/* GTK CALLBACKS */
	static void callbackCancel(GtkWidget*, TextureChooser*);
	static void callbackOK(GtkWidget*, TextureChooser*);
	static void callbackGLDraw(GtkWidget*, GdkEventExpose*, TextureChooser*);
	static void callbackSelChanged(GtkWidget*, TextureChooser*);
	
	// Update the shader attributes table
	void updateInfoTable();
	
public:

	// Construct the dialog window and its contents.
	TextureChooser(GtkWidget* combo, const std::string& prefixes);
	
	// Constructor, delete widgets
	~TextureChooser() {
		gtk_widget_destroy(_widget);
	}
	
};

}

#endif /*TEXTURECHOOSER_H_*/
