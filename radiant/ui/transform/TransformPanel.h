#pragma once

#include <string>
#include <map>
#include "iselection.h"
#include <sigc++/connection.h>

#include "wxutil/DockablePanel.h"

namespace wxutil { class ControlButton; }

class wxTextCtrl;

/**
 * The panel providing the Free Transform functionality.
 *
 * Gets notified upon selection change and updates the widget sensitivity accordingly.
 * E.g. if any entity is part of the selection, the scale widgets get disabled.
 */
namespace ui
{

class TransformPanel :
	public wxutil::DockablePanel
{
private:
	// The entry fields
	struct EntryRow
	{
		bool isRotator;
		int axis;
		int direction; // Direction (rotation only), is 1 by default
		wxTextCtrl* stepEntry;
		wxutil::ControlButton* smaller;
		wxutil::ControlButton* larger;
	};

	typedef std::map<std::string, EntryRow> EntryRowMap;
	EntryRowMap _entries;

	// The reference to the SelectionInfo (number of patches, etc.)
	const SelectionInfo& _selectionInfo;

	wxPanel* _rotatePanel;
	wxPanel* _scalePanel;

	sigc::connection _selectionChanged;

public:
	TransformPanel(wxWindow* parent);
	~TransformPanel() override;

protected:
    void onPanelActivated() override;
    void onPanelDeactivated() override;

private:
    void connectListeners();
    void disconnectListeners();

	/** greebo: Updates the sensitivity of the widgets according to
	 * 			the current selection.
	 */
	void update();

	// This is called to initialise the dialog window / create the widgets
	void populateWindow();

	/** greebo: Helper method that creates a single row
	 *
	 * @row: the row index of <table> where the widgets should be packed into.
	 * @isRotator: set to true if a rotator row is to be created.
	 * @axis: the axis this transformation is referring to.
	 */
	EntryRow createEntryRow(const std::string& label, wxSizer* table,
							bool isRotator, int axis);

	// Callbacks to catch the scale/rotation button clicks
	void onClickSmaller(wxCommandEvent& ev, EntryRow* row);
	void onClickLarger(wxCommandEvent& ev, EntryRow* row);
};

} // namespace

