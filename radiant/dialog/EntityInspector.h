#ifndef ENTITYINSPECTOR_H_
#define ENTITYINSPECTOR_H_

#include <gtk/gtk.h>

namespace ui {

/* The EntityInspector class represents the GTK dialog for editing properties
 * on the selected game entity. The class is implemented as a singleton and
 * contains a method to return the current instance.
 */

class EntityInspector
{
    // The Gtk dialog widgets
    GtkWidget* _widget;
    
    GtkWidget* _editorFrame;
    GtkWidget* _selectionTreeView;

    // Utility functions to construct the Gtk components
    void constructUI();

    GtkWidget* createDialogPane(); // bottom widget pane 
    void createSelectionTreeView(); // tree view for selecting attributes

public:

    // Return or create the singleton instance
    static EntityInspector* getInstance();

    // Get the Gtk Widget for display in the main application
    GtkWidget* getWidget();

	EntityInspector();
	virtual ~EntityInspector();
};

} // namespace ui

#endif /*ENTITYINSPECTOR_H_*/
