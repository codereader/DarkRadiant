#ifndef SOUNDCHOOSER_H_
#define SOUNDCHOOSER_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtktreeselection.h>

#include "ui/common/SoundShaderPreview.h"
#include <string>

namespace ui
{

/**
 * Dialog for listing and selection of sound shaders.
 */
class SoundChooser
{
	// Main dialog widget
	GtkWidget* _widget;

	// Tree store for shaders, and the tree selection
	GtkTreeStore* _treeStore;
	GtkTreeSelection* _treeSelection;
	
	// The preview widget group
	SoundShaderPreview _preview;
	
	// Last selected shader
	std::string _selectedShader;
	
private:

	// Widget construction
	GtkWidget* createTreeView();
	GtkWidget* createButtons();

	/* GTK CALLBACKS */
	static gboolean _onDelete(GtkWidget* w, GdkEvent* e, SoundChooser* self);
	static void _onOK(GtkWidget*, SoundChooser*);
	static void _onCancel(GtkWidget*, SoundChooser*);
	static void _onSelectionChange(GtkTreeSelection*, SoundChooser*);
	
public:
	
	/**
	 * Constructor creates widgets.
	 * 
	 * @parent
	 * The parent window.
	 */
	SoundChooser();

	/**
	 * Display the dialog and return the selection.
	 */
	std::string chooseSound();	
	
};

}

#endif /*SOUNDCHOOSER_H_*/
