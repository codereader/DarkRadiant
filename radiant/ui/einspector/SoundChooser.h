#ifndef SOUNDCHOOSER_H_
#define SOUNDCHOOSER_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtktreestore.h>

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

	// Tree store for shaders
	GtkTreeStore* _treeStore;
	
private:

	// Widget construction
	GtkWidget* createTreeView();

	/* GTK CALLBACKS */
	static void _onDelete(GtkWidget*, SoundChooser*);
	
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
