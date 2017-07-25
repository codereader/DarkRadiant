#pragma once

#include <map>
#include "icommandsystem.h"
#include "iselection.h"
#include "iradiant.h"
#include "ipatch.h"

#include <sigc++/connection.h>
#include <sigc++/trackable.h>

#include "wxutil/window/TransientWindow.h"
#include "wxutil/event/SingleIdleCallback.h"
#include "wxutil/XmlResourceBasedWidget.h"

class Patch;
class PatchNode;
typedef std::shared_ptr<PatchNode> PatchNodePtr;
typedef std::weak_ptr<PatchNode> PatchNodeWeakPtr;

namespace wxutil { class ControlButton; }
class wxChoice;
class wxPanel;
class wxTextCtrl;
class wxSizer;

namespace ui
{

// Forward declaration
class PatchInspector;
typedef std::shared_ptr<PatchInspector> PatchInspectorPtr;

class PatchInspector : 
	public wxutil::TransientWindow,
	public SelectionSystem::Observer,
	public IPatch::Observer,
	private wxutil::XmlResourceBasedWidget,
	public sigc::trackable
{
private:
	wxChoice* _rowCombo;
	wxChoice* _colCombo;
	
	struct CoordRow
	{
		wxTextCtrl* value;
		wxutil::ControlButton* smaller;
		wxutil::ControlButton* larger;
		wxTextCtrl* stepEntry;
	};

	// This are the named manipulator rows (x, y, z, s, t)
	typedef std::map<std::string, CoordRow> CoordMap;
	CoordMap _coords;

	const SelectionInfo& _selectionInfo;

	std::size_t _patchRows;
	std::size_t _patchCols;

	// The pointer to the active patch node (only non-NULL if there is a single patch selected)
	PatchNodeWeakPtr _patch;

	// If this is set to TRUE, the GTK callbacks will be disabled
	bool _updateActive;
	bool _updateNeeded;

	sigc::connection _undoHandler;
	sigc::connection _redoHandler;

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
	 */
	CoordRow createCoordRow(const std::string& label, wxPanel* parent, wxSizer* sizer);

	// Creates and packs the widgets into the dialog (called by constructor)
	void populateWindow();

	// gtkmm callbacks
	void onComboBoxChange();

	// Gets called if the spin buttons with the coordinates get changed
	void onCoordChange(wxCommandEvent& ev);
	void onStepChanged();

	void onClickSmaller(CoordRow& row);
	void onClickLarger(CoordRow& row);

	// Gets called when the "Fixed Tesselation" settings are changed
	void onFixedTessChange(wxCommandEvent& ev);
	void onTessChange(wxSpinEvent& ev);

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

	// Called by wxWidgets when the system is idle
	void onIdle(wxIdleEvent& ev);

}; // class PatchInspector

} // namespace ui
