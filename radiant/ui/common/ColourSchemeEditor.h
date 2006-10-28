#ifndef COLOURSCHEMEEDITOR_H_
#define COLOURSCHEMEEDITOR_H_

#include <gtk/gtk.h>
#include <string>
#include "colourscheme/ColourScheme.h"

namespace ui
{

// Constants
namespace {

    const int EDITOR_DEFAULT_SIZE_X = 650;
    const int EDITOR_DEFAULT_SIZE_Y = 350;
    const int TREE_VIEW_WIDTH = 180;
    
    const int COLOURS_PER_COLUMN = 7;
    
    const std::string EDITOR_WINDOW_TITLE = "Edit Colour Schemes";
}
	
class ColourSchemeEditor
{
	private:
		// The widget representing the actual editor window
		GtkWidget* _editorWidget;
		
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
		
		// Destructor
		~ColourSchemeEditor() {};
		
	private:
		// private helper functions
		void 		populateTree();
        void 		createTreeView();
		GtkWidget* 	constructWindow();
		GtkWidget* 	constructButtons();
		GtkWidget* 	constructTreeviewButtons();
		GtkWidget* 	constructColourSelector(ColourItem& colour);
		void 		updateColourSelectors();
		
		// Queries the user for a string and returns it
		// Returns "" if the user aborts or nothing is entered
		std::string inputDialog(const std::string& title, const std::string& label);
		
		// Puts the cursor on the currently active scheme
		void 		selectActiveScheme();
		
		// Updates the window after a selection change
		void 		selectionChanged();
		
		// Returns the name of the currently selected scheme
		std::string	getSelectedScheme();
		
		// Deletes or copies a scheme
		void 		deleteScheme();
		void 		copyScheme();
		
		// Deletes a scheme from the list store (called from deleteScheme())
		void 		deleteSchemeFromList();
		
		// Cleans up the widget and destroys it 
		void 		destroy();
		
		// GTK Callbacks
		static void callbackSelChanged(GtkWidget* widget, ColourSchemeEditor* self);
		static void callbackOK(GtkWidget* widget, ColourSchemeEditor* self);
		static void callbackCancel(GtkWidget* widget, ColourSchemeEditor* self);
		static void callbackColorChanged(GtkWidget* widget, ColourItem* colour);
		static void callbackDelete(GtkWidget* widget, ColourSchemeEditor* self);
		static void callbackCopy(GtkWidget* widget, ColourSchemeEditor* self);
		
}; // class ColourSchemeEditor

} // namespace ui

#endif /*COLOURSCHEMEEDITOR_H_*/
