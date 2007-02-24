#ifndef PATCHINSPECTOR_H_
#define PATCHINSPECTOR_H_

#include <map>
#include "iselection.h"
#include "gtkutil/WindowPosition.h"

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkTable GtkTable;
typedef struct _GtkObject GtkObject;

class Patch;

namespace ui {

class PatchInspector :
	public SelectionSystem::Observer
{
	// The actual dialog widget
	GtkWidget* _dialog;
	
	// The window position tracker
	gtkutil::WindowPosition _windowPosition;

	struct VertexChooser {
		GtkTable* table;
		GtkWidget* rowLabel;
		GtkWidget* colLabel;
		GtkWidget* rowCombo;
		GtkWidget* colCombo;
	} _vertexChooser;
	
	struct CoordRow {
		GtkWidget* label;
		GtkWidget* entry;
	};
	
	// This is where the named coord rows (x,y,z,s,t) are stored 
	typedef std::map<std::string, CoordRow> CoordMap;
	CoordMap _coords;
	
	GtkWidget* _coordsLabel;
	GtkTable* _coordsTable;
	
	struct TessWidgets {
		GtkWidget* title;
		GtkTable* table;
		GtkWidget* fixed;
		GtkWidget* horiz;
		GtkWidget* vert;
		GtkWidget* horizLabel;
		GtkWidget* vertLabel;
	} _tesselation;

	const SelectionInfo& _selectionInfo;

	std::size_t _patchRows;
	std::size_t _patchCols;
	
	// The pointer to the active patch
	Patch* _patch;
	
	// If this is set to TRUE, the GTK callbacks will be disabled
	bool _updateActive;

public:
	PatchInspector();
	
	/** greebo: Contains the static instance of this dialog.
	 * Constructs the instance and calls toggle() when invoked.
	 */
	static PatchInspector& Instance();

	/** greebo: This toggles the visibility of the patch inspector.
	 * The dialog is constructed only once and never destructed 
	 * during runtime.
	 */
	void toggle();

	/** greebo: SelectionSystem::Observer implementation. Gets called by
	 * the SelectionSystem upon selection change to allow updating of the
	 * patch property widgets.
	 */
	void selectionChanged(scene::Instance& instance);

	// Updates the widgets
	void update();

	/** greebo: Safely disconnects this dialog from all systems 
	 * 			(SelectionSystem, EventManager, ...)
	 * 			Also saves the window state to the registry.
	 */
	void shutdown();

private:

	/** greebo: Reloads the relevant information from the selected control vertex.
	 */
	void loadControlVertex();

	/** greebo: Saves the current values of the entry fields to the active patch control
	 */
	void emitCoords();

	/** greebo: Helper method to create an coord row (label+entry)
	 * 
	 * @tableRow: the row index of _coordsTable to pack the row into.
	 */
	CoordRow createCoordRow(const std::string& label, int tableRow);

	// Creates and packs the widgets into the dialog (called by constructor)
	void populateWindow();

	// The callback for the delete event (toggles the visibility)
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, PatchInspector* self);
	
	static void onComboBoxChange(GtkWidget* combo, PatchInspector* self);
	
	static gboolean onEntryKeyPress(GtkWidget* entry, GdkEventKey* event, PatchInspector* self);

}; // class PatchInspector

} // namespace ui

#endif /*PATCHINSPECTOR_H_*/
