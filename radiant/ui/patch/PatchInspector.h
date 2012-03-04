#ifndef PATCHINSPECTOR_H_
#define PATCHINSPECTOR_H_

#include <map>
#include "icommandsystem.h"
#include "iselection.h"
#include "iradiant.h"
#include "iundo.h"
#include "ipatch.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/event/SingleIdleCallback.h"

class Patch;
class PatchNode;
typedef boost::shared_ptr<PatchNode> PatchNodePtr;
typedef boost::weak_ptr<PatchNode> PatchNodeWeakPtr;

// Forward decls.
namespace Gtk
{
	class Table;
	class HBox;
	class Entry;
	class Label;
	class ComboBoxText;
	class SpinButton;
	class CheckButton;
}

namespace gtkutil { class ControlButton; }

namespace ui
{

// Forward declaration
class PatchInspector;
typedef boost::shared_ptr<PatchInspector> PatchInspectorPtr;

class PatchInspector
: public gtkutil::PersistentTransientWindow,
  public SelectionSystem::Observer,
  public UndoSystem::Observer,
  public gtkutil::SingleIdleCallback,
  public IPatch::Observer
{
	// The window position tracker
	gtkutil::WindowPosition _windowPosition;

	struct VertexChooser
	{
		Gtk::Table* table;
		Gtk::Label* title;
		Gtk::Label* rowLabel;
		Gtk::Label* colLabel;
		Gtk::ComboBoxText* rowCombo;
		Gtk::ComboBoxText* colCombo;
	} _vertexChooser;

	struct CoordRow
	{
		Gtk::HBox* hbox;
		Gtk::Label* label;
		Gtk::Entry* value;
		gtkutil::ControlButton* smaller;
		gtkutil::ControlButton* larger;
		Gtk::Entry* stepEntry;
		Gtk::Label* steplabel;
	};

	// This are the named manipulator rows (x, y, z, s, t)
	typedef std::map<std::string, CoordRow> CoordMap;
	CoordMap _coords;

	Gtk::Label* _coordsLabel;
	Gtk::Table* _coordsTable;

	struct TesselationWidgets
	{
		Gtk::Label* title;
		Gtk::Table* table;
		Gtk::CheckButton* fixed;
		Gtk::SpinButton* horiz;
		Gtk::SpinButton* vert;
		Gtk::Label* horizLabel;
		Gtk::Label* vertLabel;
	} _tesselation;

	const SelectionInfo& _selectionInfo;

	std::size_t _patchRows;
	std::size_t _patchCols;

	// The pointer to the active patch node (only non-NULL if there is a single patch selected)
	PatchNodeWeakPtr _patch;

	// If this is set to TRUE, the GTK callbacks will be disabled
	bool _updateActive;

private:

	// TransientWindow callbacks
	void _preShow();
	void _preHide();

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
	CoordRow createCoordRow(const std::string& label, Gtk::Table& table, int row);

	// Creates and packs the widgets into the dialog (called by constructor)
	void populateWindow();

	// gtkmm callbacks
	void onComboBoxChange();

	// Gets called if the spin buttons with the coordinates get changed
	void onCoordChange();
	void onStepChanged();

	void onClickSmaller(CoordRow* row);
	void onClickLarger(CoordRow* row);

	// Gets called when the "Fixed Tesselation" settings are changed
	void onFixedTessChange();
	void onTessChange();

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

	// Patch::Observer
	void onPatchControlPointsChanged();
	void onPatchTextureChanged();
	void onPatchDestruction();

private:
	void setPatch(const PatchNodePtr& patch);

	// Updates the widgets (this is private, use queueUpdate() instead)
	void update();

	// Relies on _patchRows/_patchCols being set correctly
	void clearVertexChooser();

	// Populates the combo boxes
	void repopulateVertexChooser();

	// This is where the static shared_ptr of the singleton instance is held.
	static PatchInspectorPtr& InstancePtr();

}; // class PatchInspector

} // namespace ui

#endif /*PATCHINSPECTOR_H_*/
