#ifndef TEXTURECHOOSER_H_
#define TEXTURECHOOSER_H_

#include "ui/common/LightTextureSelector.h"

#include <gtk/gtk.h>

#include <string>

// Forward decls

class IShader;

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
	
	// The LightTextureSelector widget, that contains the actual selection
	// tools (treeview etc.)
	ui::LightTextureSelector _selector;
	
private:

	// Widget construction helpers
	GtkWidget* createButtons();
	
	/* GTK CALLBACKS */
	static void callbackCancel(GtkWidget*, TextureChooser*);
	static void callbackOK(GtkWidget*, TextureChooser*);
	
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
