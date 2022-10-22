#pragma once

#include <map>
#include "iselection.h"
#include "ipatch.h"

#include <sigc++/connection.h>
#include <sigc++/trackable.h>

#include "wxutil/DockablePanel.h"
#include "wxutil/event/SingleIdleCallback.h"
#include "wxutil/XmlResourceBasedWidget.h"

namespace wxutil { class ControlButton; }
class wxChoice;
class wxTextCtrl;
class wxSizer;

namespace ui
{

class PatchInspector : 
	public wxutil::DockablePanel,
    public selection::SelectionSystem::Observer,
	public IPatch::Observer,
	private wxutil::XmlResourceBasedWidget,
	public sigc::trackable,
    public wxutil::SingleIdleCallback
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
	std::weak_ptr<IPatchNode> _patch;

	// If this is set to TRUE, the GTK callbacks will be disabled
	bool _updateActive;
	bool _updateNeeded;

	sigc::connection _undoHandler;
	sigc::connection _redoHandler;

private:

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

	void onClickSmaller(CoordRow& row);
	void onClickLarger(CoordRow& row);

	// Gets called when the "Fixed Tesselation" settings are changed
	void onFixedTessChange(wxCommandEvent& ev);
	void onTessChange(wxSpinEvent& ev);

public:
	PatchInspector(wxWindow* parent);
	~PatchInspector() override;

	/** greebo: SelectionSystem::Observer implementation. Gets called by
	 * the SelectionSystem upon selection change to allow updating of the
	 * patch property widgets.
	 */
	void selectionChanged(const scene::INodePtr& node, bool isComponent) override;

	// Request a deferred update of the UI elements (is performed when GTK is idle)
	void queueUpdate();

	// Patch::Observer
	void onPatchControlPointsChanged() override;
	void onPatchTextureChanged() override;
	void onPatchDestruction() override;

protected:
    void onIdle() override;

    void onPanelActivated() override;
    void onPanelDeactivated() override;

private:
    void connectEventHandlers();
    void disconnectEventHandlers();

	void setPatch(const IPatchNodePtr& patch);

	// Updates the widgets (this is private, use queueUpdate() instead)
	void update();

	// Relies on _patchRows/_patchCols being set correctly
	void clearVertexChooser();

	// Populates the combo boxes
	void repopulateVertexChooser();
};

} // namespace ui
