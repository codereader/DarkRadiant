#ifndef COLOURSCHEMEEDITOR_H_
#define COLOURSCHEMEEDITOR_H_

#include "gtkutil/window/BlockingTransientWindow.h"
#include <string>
#include "ColourScheme.h"
#include <gdk/gdkevents.h>

typedef struct _GtkWidget GtkWidget; 
typedef struct _GtkTreeSelection GtkTreeSelection;
typedef struct _GtkListStore GtkListStore;

namespace ui {

// Constants
namespace {

    const int EDITOR_DEFAULT_SIZE_X = 650;
    const int EDITOR_DEFAULT_SIZE_Y = 450;
    
    const int COLOURS_PER_COLUMN = 10;
    
    const std::string EDITOR_WINDOW_TITLE = "Edit Colour Schemes";
}
	
class ColourSchemeEditor :
	public gtkutil::BlockingTransientWindow
{
private:
	// The treeview and its selection pointer
	GtkWidget* _treeView;
	GtkTreeSelection* _selection;
	
	// The list store containing the list of ColourSchemes		
	GtkListStore* _listStore;
	
	// The vbox containing the colour buttons and its frame
	GtkWidget* _colourBox;
	GtkWidget* _colourFrame;
	
	// The "delete scheme" button
	GtkWidget* _deleteButton;

public:
	// Constructor
	ColourSchemeEditor();
	
private:
	// private helper functions
	void 		populateTree();
    void 		createTreeView();
	GtkWidget* 	constructWindow();
	GtkWidget* 	constructButtons();
	GtkWidget* 	constructTreeviewButtons();
	GtkWidget* 	constructColourSelector(ColourItem& colour, const std::string& name);
	void 		updateColourSelectors();
	
	// Queries the user for a string and returns it
	// Returns "" if the user aborts or nothing is entered
	std::string inputDialog(const std::string& title, const std::string& label);
	
	// Puts the cursor on the currently active scheme
	void 		selectActiveScheme();
	
	// Updates the colour selectors after a selection change
	void 		selectionChanged();
	
	// Returns the name of the currently selected scheme
	std::string	getSelectedScheme();
	
	// Deletes or copies a scheme
	void 		deleteScheme();
	void 		copyScheme();
	
	// Deletes a scheme from the list store (called from deleteScheme())
	void 		deleteSchemeFromList();
	
	// GTK Callbacks
	static void callbackSelChanged(GtkWidget* widget, ColourSchemeEditor* self);
	static void callbackOK(GtkWidget* widget, ColourSchemeEditor* self);
	static void callbackCancel(GtkWidget* widget, ColourSchemeEditor* self);
	static void callbackColorChanged(GtkWidget* widget, ColourItem* colour);
	static void callbackDelete(GtkWidget* widget, ColourSchemeEditor* self);
	static void callbackCopy(GtkWidget* widget, ColourSchemeEditor* self);
	static void _onDeleteEvent(GtkWidget*, GdkEvent*, ColourSchemeEditor*);
	
	// Destroy window and delete self, called by both Cancel and window
	// delete callbacks
	void doCancel();
	
	// Updates the windows after a colour change
	static void updateWindows();
	
public:
	// Command target
	static void editColourSchemes();
}; // class ColourSchemeEditor

} // namespace ui

#endif /*COLOURSCHEMEEDITOR_H_*/
