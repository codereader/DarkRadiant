#ifndef PATCHINSPECTOR_H_
#define PATCHINSPECTOR_H_

#include <map>
#include "icommandsystem.h"
#include "iselection.h"
#include "iradiant.h"
#include "iundo.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/RegistryConnector.h"
#include "gtkutil/event/SingleIdleCallback.h"

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkTable GtkTable;
typedef struct _GtkObject GtkObject;
typedef struct _GtkEditable GtkEditable;
namespace gtkutil { class ControlButton; }
class Patch;

namespace ui {

// Forward declaration
class PatchInspector;
typedef boost::shared_ptr<PatchInspector> PatchInspectorPtr;

class PatchInspector 
: public gtkutil::PersistentTransientWindow,
  public SelectionSystem::Observer,
  public RadiantEventListener,
  public UndoSystem::Observer,
  public gtkutil::SingleIdleCallback
{
	// The window position tracker
	gtkutil::WindowPosition _windowPosition;

	struct VertexChooser {
		GtkTable* table;
		GtkWidget* title;
		GtkWidget* rowLabel;
		GtkWidget* colLabel;
		GtkWidget* rowCombo;
		GtkWidget* colCombo;
	} _vertexChooser;
	
	typedef boost::shared_ptr<gtkutil::ControlButton> ControlButtonPtr;
	
	struct CoordRow {
		GtkWidget* hbox;
		GtkWidget* label;
		GtkWidget* value;
		gulong valueChangedHandler;
		ControlButtonPtr smaller; 
		ControlButtonPtr larger;
		GtkWidget* step;
		GtkWidget* steplabel;
	};
	
	// This are the named manipulator rows (x, y, z, s, t) 
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
	
	// The pointer to the active patch (only non-NULL if there is a single patch selected)
	Patch* _patch;
	
	// If this is set to TRUE, the GTK callbacks will be disabled
	bool _updateActive;

	// The helper class that syncs the registry to/from widgets
	gtkutil::RegistryConnector _connector;

private:

	// TransientWindow callbacks
	void _preShow();
	void _preHide();
	
	/** greebo: This toggles the visibility of the patch inspector.
	 * The dialog is constructed only once and never destructed 
	 * during runtime.
	 */
	void toggleWindow();

	/** greebo: Saves the step values to the registry
	 */
	void saveToRegistry();

	/** greebo: Helper method that imports the selected patch
	 */
	void rescanSelection();

	/** greebo: Reloads the relevant information from the selected control vertex.
	 */
	void loadControlVertex();

	/** greebo: Saves the current values of the entry fields to the active patch control
	 */
	void emitCoords();
	
	/** greebo: Writes the tesselation setting into the selected patch
	 */
	void emitTesselation();

	/** greebo: Helper method to create an coord row (label+entry)
	 * 
	 * @table: The GtkTable the widgets should be packed in
	 * @row: the row index of _coordsTable to pack the row into.
	 */
	CoordRow createCoordRow(const std::string& label, GtkTable* table, int row);

	// Creates and packs the widgets into the dialog (called by constructor)
	void populateWindow();

	static void onComboBoxChange(GtkWidget* combo, PatchInspector* self);
	
	// Gets called if the spin buttons with the coordinates get changed
	static void onCoordChange(GtkEditable* editable, PatchInspector* self);
	static void onStepChanged(GtkEditable* editable, PatchInspector* self);
	
	static void onClickSmaller(GtkWidget* button, CoordRow* row);
	static void onClickLarger(GtkWidget* button, CoordRow* row);

	// Gets called when the "Fixed Tesselation" settings are changed 
	static void onFixedTessChange(GtkWidget* checkButton, PatchInspector* self);
	static void onTessChange(GtkEditable* editable, PatchInspector* self);

public:
	PatchInspector();
	
	/** greebo: Contains the static instance of this dialog.
	 * Constructs the instance and calls toggle() when invoked.
	 */
	static PatchInspector& Instance();

	// The command target
	static void toggle(const cmd::ArgumentList& args);

	/** greebo: SelectionSystem::Observer implementation. Gets called by
	 * the SelectionSystem upon selection change to allow updating of the
	 * patch property widgets.
	 */
	void selectionChanged(const scene::INodePtr& node, bool isComponent);

	// Request a deferred update of the UI elements (is performed when GTK is idle)
	void queueUpdate();

	/** 
	 * greebo: Safely disconnects this dialog from all systems 
	 * (SelectionSystem, EventManager, ...)
	 * Also saves the window state to the registry.
	 */
	void onRadiantShutdown();

	// UndoSystem::Observer implementation
	void postUndo();
	void postRedo();

	// Idle callback, used for deferred updates
	void onGtkIdle();

private:
	// Updates the widgets (this is private, use queueUpdate() instead)
	void update();

	// This is where the static shared_ptr of the singleton instance is held.
	static PatchInspectorPtr& InstancePtr();

}; // class PatchInspector

} // namespace ui

#endif /*PATCHINSPECTOR_H_*/
