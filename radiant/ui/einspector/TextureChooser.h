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
	
private:

	// Widget construction helpers
	GtkWidget* createTreeView();
	GtkWidget* createButtons();
	
public:

	// Construct the dialog window and its contents.
	TextureChooser(GtkWidget* combo);
	
};

}

#endif /*TEXTURECHOOSER_H_*/
