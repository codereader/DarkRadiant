#include "TransformPanel.h"

#include "i18n.h"
#include "icommandsystem.h"
#include "selectionlib.h"
#include "registry/Widgets.h"

#include "wxutil/ControlButton.h"
#include "wxutil/dialog/MessageBox.h"

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include "wxutil/Bitmap.h"
#include <functional>

namespace ui
{

namespace 
{
	constexpr const char* const LABEL_ROTATION = N_("Rotation");
	constexpr const char* const LABEL_SCALE = N_("Scale");

	constexpr const char* const LABEL_ROTX = N_("X-Axis Rotate:");
	constexpr const char* const LABEL_ROTY = N_("Y-Axis Rotate:");
	constexpr const char* const LABEL_ROTZ = N_("Z-Axis Rotate:");

	constexpr const char* const LABEL_SCALEX = N_("X-Axis Scale:");
	constexpr const char* const LABEL_SCALEY = N_("Y-Axis Scale:");
	constexpr const char* const LABEL_SCALEZ = N_("Z-Axis Scale:");

    constexpr const char* const LABEL_STEP = N_("Step:");

	const std::string RKEY_ROOT = "user/ui/transformPanel/";
	const std::string RKEY_ROTX_STEP = RKEY_ROOT + "rotXStep";
	const std::string RKEY_ROTY_STEP = RKEY_ROOT + "rotYStep";
	const std::string RKEY_ROTZ_STEP = RKEY_ROOT + "rotZStep";

	const std::string RKEY_SCALEX_STEP = RKEY_ROOT + "scaleXStep";
	const std::string RKEY_SCALEY_STEP = RKEY_ROOT + "scaleYStep";
	const std::string RKEY_SCALEZ_STEP = RKEY_ROOT + "scaleZStep";
}

TransformPanel::TransformPanel(wxWindow* parent) :
    DockablePanel(parent),
    _selectionInfo(GlobalSelectionSystem().getSelectionInfo())
{
	// Create all the widgets and pack them into the window
	populateWindow();
}

TransformPanel::~TransformPanel()
{
    if (panelIsActive())
    {
        disconnectListeners();
    }
}

void TransformPanel::onPanelActivated()
{
    connectListeners();

    // Update the widget sensitivity
    update();
}

void TransformPanel::onPanelDeactivated()
{
    disconnectListeners();
}

void TransformPanel::connectListeners()
{
    // Register self to the SelSystem to get notified upon selection changes.
    _selectionChanged = GlobalSelectionSystem().signal_selectionChanged().connect(
        [this](const ISelectable&) { update(); });
}

void TransformPanel::disconnectListeners()
{
    _selectionChanged.disconnect();
}

void TransformPanel::populateWindow()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* overallVBox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(overallVBox, 1, wxEXPAND | wxALL, 12);

	_rotatePanel = new wxPanel(this, wxID_ANY);
	_scalePanel = new wxPanel(this, wxID_ANY);

	overallVBox->Add(_rotatePanel, 0, wxEXPAND);
	overallVBox->Add(_scalePanel, 1, wxEXPAND);

	_rotatePanel->SetSizer(new wxBoxSizer(wxVERTICAL));
	_scalePanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	// Create the rotation label (bold font)
	wxStaticText* rotateLabel = new wxStaticText(_rotatePanel, wxID_ANY, _(LABEL_ROTATION));
	rotateLabel->SetFont(rotateLabel->GetFont().Bold());

	// Create the scale label (bold font)
	wxStaticText* scaleLabel = new wxStaticText(_scalePanel, wxID_ANY, _(LABEL_SCALE));
	scaleLabel->SetFont(scaleLabel->GetFont().Bold());
	
	// Arrange label and control rows
	wxFlexGridSizer* rotateTable = new wxFlexGridSizer(3, 4, 3, 6);
	wxFlexGridSizer* scaleTable = new wxFlexGridSizer(3, 4, 3, 6);

	_rotatePanel->GetSizer()->Add(rotateLabel, 0, wxBOTTOM, 6);
	_rotatePanel->GetSizer()->Add(rotateTable, 1, wxEXPAND | wxLEFT, 6);

	_scalePanel->GetSizer()->Add(scaleLabel, 0, wxTOP | wxBOTTOM, 6);
	_scalePanel->GetSizer()->Add(scaleTable, 1, wxEXPAND | wxLEFT, 6);

    _entries["rotateX"] = createEntryRow(_(LABEL_ROTX), rotateTable, true, 0);
    _entries["rotateY"] = createEntryRow(_(LABEL_ROTY), rotateTable, true, 1);
    _entries["rotateZ"] = createEntryRow(_(LABEL_ROTZ), rotateTable, true, 2);

	_entries["scaleX"] = createEntryRow(_(LABEL_SCALEX), scaleTable, false, 0);
    _entries["scaleY"] = createEntryRow(_(LABEL_SCALEY), scaleTable, false, 1);
    _entries["scaleZ"] = createEntryRow(_(LABEL_SCALEZ), scaleTable, false, 2);

    // Connect the step values to the according registry values
    registry::bindWidget(_entries["rotateX"].stepEntry, RKEY_ROTX_STEP);
    registry::bindWidget(_entries["rotateY"].stepEntry, RKEY_ROTY_STEP);
    registry::bindWidget(_entries["rotateZ"].stepEntry, RKEY_ROTZ_STEP);

    registry::bindWidget(_entries["scaleX"].stepEntry, RKEY_SCALEX_STEP);
    registry::bindWidget(_entries["scaleY"].stepEntry, RKEY_SCALEY_STEP);
    registry::bindWidget(_entries["scaleZ"].stepEntry, RKEY_SCALEZ_STEP);

    // Connect all the arrow buttons
    for (EntryRowMap::iterator i = _entries.begin(); i != _entries.end(); ++i)
	{
    	EntryRow& row = i->second;

		// Pass a EntryRow pointer to the callback, that's all it will need to update
		row.smaller->Bind(wxEVT_BUTTON, std::bind(&TransformPanel::onClickSmaller, this, std::placeholders::_1, &row));
		row.larger->Bind(wxEVT_BUTTON, std::bind(&TransformPanel::onClickLarger, this, std::placeholders::_1, &row));
    }
}

TransformPanel::EntryRow TransformPanel::createEntryRow(
	const std::string& label, wxSizer* table, bool isRotator, int axis)
{
	EntryRow entryRow;

	entryRow.isRotator = isRotator;
	entryRow.axis = axis;

	// greebo: The rotation direction is reversed for X and Z rotations
	// This has no mathematical meaning, it's just for looking right.
	entryRow.direction = (isRotator && axis != 1) ? -1 : 1;

	wxWindow* parent = table->GetContainingWindow();

	// Create the label
	wxStaticText* labelWidget = new wxStaticText(parent, wxID_ANY, label);

	table->Add(labelWidget, 0, wxALIGN_CENTER_VERTICAL);

	// Create the control buttons (zero spacing hbox)
	{
		wxBoxSizer* controlButtonBox = new wxBoxSizer(wxHORIZONTAL);
		controlButtonBox->SetMinSize(30, 30);

		entryRow.smaller = new wxutil::ControlButton(parent, 
			wxutil::GetLocalBitmap("arrow_left.png"));
		entryRow.smaller->SetMinSize(wxSize(15, 24));
		controlButtonBox->Add(entryRow.smaller, 0);

		entryRow.larger = new wxutil::ControlButton(parent, 
			wxutil::GetLocalBitmap("arrow_right.png"));
		entryRow.larger->SetMinSize(wxSize(15, 24));
		controlButtonBox->Add(entryRow.larger, 0);
		
		table->Add(controlButtonBox, 0, wxLEFT, 6);
	}
	
	// Create the label
	wxStaticText* stepLabel = new wxStaticText(parent, wxID_ANY, _(LABEL_STEP));

	table->Add(stepLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);

	// Create the entry field
	entryRow.stepEntry = new wxTextCtrl(parent, wxID_ANY);
	entryRow.stepEntry->SetMinClientSize(wxSize(entryRow.stepEntry->GetCharWidth() * 5, -1));

	table->Add(entryRow.stepEntry, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);

	// Return the filled structure
	return entryRow;
}

void TransformPanel::update()
{
	// Check if there is anything selected
	bool rotSensitive = (_selectionInfo.totalCount > 0);
	bool scaleSensitive = (_selectionInfo.totalCount > 0 && _selectionInfo.entityCount == 0);

	// set the sensitivity of the scale/rotation widgets
	_rotatePanel->Enable(rotSensitive);
	_rotatePanel->Enable(rotSensitive);

	_scalePanel->Enable(scaleSensitive);
	_scalePanel->Enable(scaleSensitive);
}

void TransformPanel::onClickLarger(wxCommandEvent& ev, EntryRow* row)
{
	// Get the current step increment
	float step = string::convert<float>(row->stepEntry->GetValue().ToStdString());

	// Determine the action
	if (row->isRotator)
	{
		// Do a rotation
		Vector3 eulerXYZ;

		// Store the value into the right axis
		eulerXYZ[row->axis] = step * row->direction;

		// Pass the call to the algorithm functions
		GlobalCommandSystem().executeCommand("RotateSelectedEulerXYZ", eulerXYZ);
	}
	else
	{
		// Do a scale
		Vector3 scaleXYZ(1,1,1);

		// Store the value into the right axis
		scaleXYZ[row->axis] = step;

		// Pass the call to the algorithm functions
		GlobalCommandSystem().executeCommand("ScaleSelected", scaleXYZ);
	}
}

void TransformPanel::onClickSmaller(wxCommandEvent& ev, EntryRow* row)
{
	// Get the current value and the step increment
	float step = string::convert<float>(row->stepEntry->GetValue().ToStdString());

	// Determine the action
	if (row->isRotator)
	{
		// Do a rotation
		Vector3 eulerXYZ;

		// Store the value into the right axis
		eulerXYZ[row->axis] = -step * row->direction;

		// Pass the call to the algorithm functions
		GlobalCommandSystem().executeCommand("RotateSelectedEulerXYZ", eulerXYZ);
	}
	else
	{
		// Do a scale
		if (float_equal_epsilon(step, 0.0f, 0.0001f))
		{
			wxutil::Messagebox::ShowError(_("Cannot scale by zero or near-zero values"));
			return;
		}
		
		Vector3 scaleXYZ(1,1,1);

		// Store the value into the right axis
		scaleXYZ[row->axis] = 1/step;

		// Pass the call to the algorithm functions
		GlobalCommandSystem().executeCommand("ScaleSelected", scaleXYZ);
	}
}

} // namespace
