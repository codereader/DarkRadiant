#ifndef SURFACEINSPECTOR_H_
#define SURFACEINSPECTOR_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtktable.h>
#include <map>

namespace ui {

class SurfaceInspector
{
	// The actual dialog window
	GtkWidget* _dialog;

	struct ManipulatorRow {
		GtkWidget* label;
		GtkWidget* value;
		GtkWidget* leftArrow; 
		GtkWidget* rightArrow;
		GtkWidget* step;
		GtkWidget* steplabel;
	};

	// This are the named manipulator rows (shift, scale, rotation, etc) 
	typedef std::map<std::string, ManipulatorRow> ManipulatorMap;
	ManipulatorMap _manipulators;

	// The "shader" entry field
	GtkWidget* _shaderEntry;
	
	struct FitTextureWidgets {
		GtkWidget* width;
		GtkWidget* height;
		GtkWidget* button;
	} _fitTexture;

public:

	// Constructor
	SurfaceInspector();

	/** greebo: Contains the static instance of this dialog.
	 * Constructs the instance and calls toggle() when invoked.
	 */
	static void toggleInspector();

private:
	/** greebo: Creates a row consisting of label, value entry,
	 * two arrow buttons and a step entry field.
	 * 
	 * @table: the GtkTable the row should be packed into.
	 * @row: the target row number (first table row = 0).
	 * 
	 * @returns: the structure containing the widget pointers. 
	 */
	ManipulatorRow createManipulatorRow(const std::string& label, 
										GtkTable* table, 
										int row);

	// Adds all the widgets to the window
	void populateWindow();

	// Shows/hides this dialog
	void toggle();
	
	// The callback for the delete event (toggles the visibility)
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, SurfaceInspector* self);
	
}; // class SurfaceInspector

} // namespace ui

#endif /*SURFACEINSPECTOR_H_*/
