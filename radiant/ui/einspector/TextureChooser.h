#ifndef TEXTURECHOOSER_H_
#define TEXTURECHOOSER_H_

#include <gtk/gtk.h>

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
	
private:

	// Widget construction helpers
	GtkWidget* createTreeView();
	GtkWidget* createButtons();
	
	/* GTK CALLBACKS */
	static void callbackCancel(GtkWidget*, TextureChooser*);
	static void callbackOK(GtkWidget*, TextureChooser*);
	
public:

	// Construct the dialog window and its contents.
	TextureChooser(GtkWidget* combo);
	
	// Constructor, delete widgets
	~TextureChooser() {
		gtk_widget_destroy(_widget);
	}
	
};

}

#endif /*TEXTURECHOOSER_H_*/
