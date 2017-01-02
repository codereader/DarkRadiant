#pragma once

#include <wx/event.h>
#include <memory>
#include <wx/colour.h>

class wxWindow;
class wxButton;
class wxToggleButton;
class wxSizer;
class wxPanel;

namespace ui
{

/**
 * greebo: A LayerControl contains a set of widgets needed
 * to control an associated layer.
 *
 * Multiple LayerControls can be packed as children into the
 * owning LayerControlDialog.
 */
class LayerControl :
	public wxEvtHandler
{
private:
	// The ID of the associated layer
	int _layerID;

	wxToggleButton* _toggle;
	wxPanel* _statusWidget;
	wxButton* _labelButton;
	wxButton* _deleteButton;
	wxButton* _renameButton;
	wxSizer* _buttonHBox;

	wxColour _activeColour;
	wxColour _inactiveColour;

	// Locks down the callbacks during widget update
	bool _updateActive;

public:
	LayerControl(wxWindow* parent, int layerID);

	// Returns the widgets for packing this object into a container/table
	wxButton* getLabelButton();
	wxWindow* getStatusWidget();
	wxSizer* getButtons();
	wxToggleButton* getToggle();

	// Updates the state of all widgets, 
	// except for the usage status widget which is reset to the "inactive" default
	void update();

	int getLayerId() const;

	void updateUsageStatusWidget(std::size_t numUsedObjectsInLayer);

private:
	void onToggle(wxCommandEvent& ev);
	void onDelete(wxCommandEvent& ev);
	void onRename(wxCommandEvent& ev);
	void onLayerSelect(wxCommandEvent& ev);
};
typedef std::shared_ptr<LayerControl> LayerControlPtr;

} // namespace ui
