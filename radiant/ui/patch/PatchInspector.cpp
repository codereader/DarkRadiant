#include "PatchInspector.h"

#include "i18n.h"
#include "itextstream.h"
#include "iuimanager.h"
#include "imainframe.h"
#include "iundo.h"

#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>

#include "registry/Widgets.h"
#include "selectionlib.h"
#include "wxutil/ControlButton.h"

#include "patch/PatchNode.h"
#include "selection/algorithm/Primitives.h"
#include <functional>

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Patch Inspector");
	const char* const LABEL_STEP = N_("Step:");

	const std::string RKEY_ROOT = "user/ui/patch/patchInspector/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
	const std::string RKEY_X_STEP = RKEY_ROOT + "xCoordStep";
	const std::string RKEY_Y_STEP = RKEY_ROOT + "yCoordStep";
	const std::string RKEY_Z_STEP = RKEY_ROOT + "zCoordStep";
	const std::string RKEY_S_STEP = RKEY_ROOT + "sCoordStep";
	const std::string RKEY_T_STEP = RKEY_ROOT + "tCoordStep";
}

PatchInspector::PatchInspector() : 
	TransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow(), true),
	_rowCombo(NULL),
	_colCombo(NULL),
	_selectionInfo(GlobalSelectionSystem().getSelectionInfo()),
	_patchRows(0),
	_patchCols(0),
	_updateActive(false),
	_updateNeeded(false)
{
	Connect(wxEVT_IDLE, wxIdleEventHandler(PatchInspector::onIdle), NULL, this);

	// Create all the widgets and pack them into the window
	populateWindow();

	// Update the widget status
	rescanSelection();

	InitialiseWindowPosition(410, 480, RKEY_WINDOW_STATE);
}

PatchInspectorPtr& PatchInspector::InstancePtr()
{
	static PatchInspectorPtr _instancePtr;
	return _instancePtr;
}

void PatchInspector::onRadiantShutdown()
{
	rMessage() << "PatchInspector shutting down." << std::endl;

	if (IsShownOnScreen())
	{
		Hide();
	}

	// Destroy the window 
	SendDestroyEvent();
	InstancePtr().reset();
}

PatchInspector& PatchInspector::Instance()
{
	PatchInspectorPtr& instancePtr = InstancePtr();

	if (instancePtr == NULL)
	{
		// Not yet instantiated, do it now
		instancePtr.reset(new PatchInspector);

		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().signal_radiantShutdown().connect(
            sigc::mem_fun(*instancePtr, &PatchInspector::onRadiantShutdown)
        );
	}

	return *instancePtr;
}

void PatchInspector::populateWindow()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));
	GetSizer()->Add(loadNamedPanel(this, "PatchInspectorMainPanel"), 1, wxEXPAND);

	makeLabelBold(this, "PatchInspectorVertexLabel");
	makeLabelBold(this, "PatchInspectorCoordLabel");
	makeLabelBold(this, "PatchInspectorTessLabel");

	_rowCombo = findNamedObject<wxChoice>(this, "PatchInspectorControlRow");
	_colCombo = findNamedObject<wxChoice>(this, "PatchInspectorControlColumn");

	// Create the controls table
	wxPanel* coordPanel = findNamedObject<wxPanel>(this, "PatchInspectorCoordPanel");
	wxFlexGridSizer* table = new wxFlexGridSizer(5, 5, 6, 16);
	table->AddGrowableCol(1);

	coordPanel->GetSizer()->Add(table, 1, wxEXPAND);

    _coords["x"] = createCoordRow("X:", coordPanel, table);
    _coords["y"] = createCoordRow("Y:", coordPanel, table);
    _coords["z"] = createCoordRow("Z:", coordPanel, table);
    _coords["s"] = createCoordRow("S:", coordPanel, table);
    _coords["t"] = createCoordRow("T:", coordPanel, table);

    // Connect the step values to the according registry values
	registry::bindWidget(_coords["x"].stepEntry, RKEY_X_STEP);
    registry::bindWidget(_coords["y"].stepEntry, RKEY_Y_STEP);
    registry::bindWidget(_coords["z"].stepEntry, RKEY_Z_STEP);
    registry::bindWidget(_coords["s"].stepEntry, RKEY_S_STEP);
    registry::bindWidget(_coords["t"].stepEntry, RKEY_T_STEP);

    // Connect all the arrow buttons
    for (CoordMap::iterator i = _coords.begin(); i != _coords.end(); ++i)
	{
    	CoordRow& row = i->second;

    	// Pass a CoordRow ref to the callback, that's all it will need to update
		row.smaller->Bind(wxEVT_BUTTON, std::bind(&PatchInspector::onClickSmaller, this, row));
		row.larger->Bind(wxEVT_BUTTON, std::bind(&PatchInspector::onClickLarger, this, row));
    }

	// Tesselation checkbox
	findNamedObject<wxCheckBox>(this, "PatchInspectorFixedSubdivisions")->Connect(
		wxEVT_CHECKBOX, wxCommandEventHandler(PatchInspector::onFixedTessChange), NULL, this);

	// Tesselation values
	findNamedObject<wxSpinCtrl>(this, "PatchInspectorSubdivisionsX")->Connect(
		wxEVT_SPINCTRL, wxSpinEventHandler(PatchInspector::onTessChange), NULL, this);
	findNamedObject<wxSpinCtrl>(this, "PatchInspectorSubdivisionsY")->Connect(
		wxEVT_SPINCTRL, wxSpinEventHandler(PatchInspector::onTessChange), NULL, this);
}

PatchInspector::CoordRow PatchInspector::createCoordRow(
	const std::string& label, wxPanel* parent, wxSizer* sizer)
{
	CoordRow coordRow;

	// Create the coordinate label
	wxStaticText* coordLabel = new wxStaticText(parent, wxID_ANY, label);

	coordRow.value = new wxTextCtrl(parent, wxID_ANY);
	coordRow.value->SetMinClientSize(wxSize(coordRow.value->GetCharWidth() * 7, -1));
	coordRow.value->Connect(wxEVT_TEXT, wxCommandEventHandler(PatchInspector::onCoordChange), NULL, this);

	// Coord control
	wxBoxSizer* controlButtonBox = new wxBoxSizer(wxHORIZONTAL);
	controlButtonBox->SetMinSize(30, 30);

	coordRow.smaller = new wxutil::ControlButton(parent, 
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "arrow_left.png"));
	coordRow.smaller->SetMinSize(wxSize(15, 24));
	controlButtonBox->Add(coordRow.smaller, 0);

	coordRow.larger = new wxutil::ControlButton(parent, 
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "arrow_right.png"));
	coordRow.larger->SetMinSize(wxSize(15, 24));
	controlButtonBox->Add(coordRow.larger, 0);

	// Create the step label
	wxStaticText* steplabel = new wxStaticText(parent, wxID_ANY, _(LABEL_STEP));

	// Create the step entry field
	coordRow.stepEntry = new wxTextCtrl(parent, wxID_ANY);
	coordRow.stepEntry->SetMinClientSize(wxSize(coordRow.stepEntry->GetCharWidth() * 5, -1));

	sizer->Add(coordLabel, 0, wxALIGN_CENTRE_VERTICAL);
	sizer->Add(coordRow.value, 0, wxALIGN_CENTRE_VERTICAL);
	sizer->Add(controlButtonBox, 0, wxALIGN_CENTRE_VERTICAL);
	sizer->Add(steplabel, 0, wxALIGN_CENTRE_VERTICAL);
	sizer->Add(coordRow.stepEntry, 0, wxALIGN_CENTRE_VERTICAL);

	// Return the filled structure
	return coordRow;
}

void PatchInspector::queueUpdate()
{
	// Request an idle callback to perform the update when GTK is idle
	_updateNeeded = true;
}

void PatchInspector::update()
{
	_updateActive = true;

	PatchNodePtr patch = _patch.lock();

	if (patch != NULL)
	{
		// Check if the matrix size has changed
		if (patch->getPatch().getWidth() != _patchCols ||
			patch->getPatch().getHeight() != _patchRows)
		{
			// Patch matrix got changed
			clearVertexChooser();
			repopulateVertexChooser();
		}

		// Load the data from the currently selected vertex
		loadControlVertex();
	}

	_updateActive = false;
}

void PatchInspector::loadControlVertex()
{
	PatchNodePtr patch = _patch.lock();

	if (patch != NULL)
	{
		int row = _rowCombo->GetSelection();
		int col = _colCombo->GetSelection();

		// Retrieve the controlvertex
		const PatchControl& ctrl = patch->getPatch().ctrlAt(row, col);

		_updateActive = true;

		_coords["x"].value->SetValue(string::to_string(ctrl.vertex[0]));
		_coords["y"].value->SetValue(string::to_string(ctrl.vertex[1]));
		_coords["z"].value->SetValue(string::to_string(ctrl.vertex[2]));
		_coords["s"].value->SetValue(string::to_string(ctrl.texcoord[0]));
		_coords["t"].value->SetValue(string::to_string(ctrl.texcoord[1]));

		_updateActive = false;
	}
}

// Pre-hide callback
void PatchInspector::_preHide()
{
	TransientWindow::_preHide();

	// Clear the patch, we don't need to observe it while hidden
	setPatch(PatchNodePtr());

	// A hidden PatchInspector doesn't need to listen for events
	_undoHandler.disconnect();
	_redoHandler.disconnect();

	GlobalSelectionSystem().removeObserver(this);
}

// Pre-show callback
void PatchInspector::_preShow()
{
	TransientWindow::_preShow();

	_undoHandler.disconnect();
	_redoHandler.disconnect();

	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);
	
	_undoHandler = GlobalUndoSystem().signal_postUndo().connect(
		sigc::mem_fun(this, &PatchInspector::queueUpdate));
	_redoHandler = GlobalUndoSystem().signal_postRedo().connect(
		sigc::mem_fun(this, &PatchInspector::queueUpdate));

	// Check for selection changes before showing the dialog again
	rescanSelection();
}

void PatchInspector::selectionChanged(const scene::INodePtr& node, bool isComponent)
{
	if (!isComponent)
	{
		rescanSelection();
	}
}

void PatchInspector::clearVertexChooser()
{
	_updateActive = true;

	// Remove all the items from the combo boxes
	_rowCombo->Clear();
	_colCombo->Clear();

	_updateActive = false;
}

void PatchInspector::setPatch(const PatchNodePtr& newPatch)
{
	// Detach if we had a previous patch
	PatchNodePtr patch = _patch.lock();

	if (patch != NULL)
	{
		patch->getPatch().detachObserver(this);
	}

	// Clear vertex chooser while _patchRows/_patchCols are still set
	clearVertexChooser();

	_patch = newPatch;

	if (newPatch != NULL)
	{
		newPatch->getPatch().attachObserver(this);

		_patchRows = newPatch->getPatch().getHeight();
		_patchCols = newPatch->getPatch().getWidth();

		// Now that rows/cols are known, build lists
		repopulateVertexChooser();
	}
	else
	{
		_patchRows = 0;
		_patchCols = 0;
	}
}

void PatchInspector::rescanSelection()
{
	// Check if there is one distinct patch selected
	bool sensitive = (_selectionInfo.patchCount == 1);

	findNamedObject<wxPanel>(this, "PatchInspectorVertexPanel")->Enable(sensitive);
	findNamedObject<wxPanel>(this, "PatchInspectorCoordPanel")->Enable(sensitive);

	// Tesselation is always sensitive when one or more patches are selected
	findNamedObject<wxPanel>(this, "PatchInspectorTessPanel")->Enable(_selectionInfo.patchCount > 0);

	// Clear the patch reference
	setPatch(PatchNodePtr());

	if (_selectionInfo.patchCount > 0)
	{
		// Get the list of selected patches
		PatchPtrVector list = selection::algorithm::getSelectedPatches();

		Subdivisions tess(UINT_MAX, UINT_MAX);
		bool tessIsFixed = false;

		// Try to find a pair of same tesselation values
		for (PatchPtrVector::const_iterator i = list.begin(); i != list.end(); ++i)
        {
			IPatch& p = (*i)->getPatch();

			if (tess.x() == UINT_MAX)
            {
				// Not initialised yet, take these values for starters
				tessIsFixed = p.subdivisionsFixed();
				tess = p.getSubdivisions();
			}
			else
			{
				// We already have a pair of divisions, compare
				Subdivisions otherTess = p.getSubdivisions();

				if (tessIsFixed != p.subdivisionsFixed() || otherTess != tess)
				{
					// Our journey ends here, we cannot find a pair of tesselations
					// for all selected patches or the same fixed/variable status
					tessIsFixed = false;
					break;
				}
			}
		}

		_updateActive = true;

		// Load the "fixed tesselation" value
		findNamedObject<wxCheckBox>(this, "PatchInspectorFixedSubdivisions")->SetValue(tessIsFixed);

		wxSpinCtrl* fixedSubdivX = findNamedObject<wxSpinCtrl>(this, "PatchInspectorSubdivisionsX");
		wxSpinCtrl* fixedSubdivY = findNamedObject<wxSpinCtrl>(this, "PatchInspectorSubdivisionsY");

		fixedSubdivX->SetValue(tess[0]);
		fixedSubdivY->SetValue(tess[1]);

		fixedSubdivX->Enable(tessIsFixed);
		fixedSubdivY->Enable(tessIsFixed);

		findNamedObject<wxStaticText>(this, "PatchInspectorSubdivisionsXLabel")->Enable(tessIsFixed);
		findNamedObject<wxStaticText>(this, "PatchInspectorSubdivisionsYLabel")->Enable(tessIsFixed);

		if (_selectionInfo.patchCount == 1)
		{
			setPatch(list[0]);
		}

		_updateActive = false;
	}

	update();
}

void PatchInspector::repopulateVertexChooser()
{
	_updateActive = true;

	for (std::size_t i = 0; i < _patchRows; ++i)
	{
		_rowCombo->AppendString(string::to_string(i));
	}

	_rowCombo->Select(0);

	for (std::size_t i = 0; i < _patchCols; ++i)
	{
		_colCombo->AppendString(string::to_string(i));
	}

	_colCombo->Select(0);

	_updateActive = false;
}

void PatchInspector::emitCoords()
{
	PatchNodePtr patch = _patch.lock();

	if (patch == NULL) return;

	// Save the coords into the patch
	UndoableCommand emitCoordsCmd("patchAdjustControlVertex");

	patch->getPatchInternal().undoSave();

	int row = _rowCombo->GetSelection();
	int col = _colCombo->GetSelection();

	// Retrieve the controlvertex
	PatchControl& ctrl = patch->getPatchInternal().ctrlAt(row, col);

	ctrl.vertex[0] = string::convert<float>(_coords["x"].value->GetValue().ToStdString());
	ctrl.vertex[1] = string::convert<float>(_coords["y"].value->GetValue().ToStdString());
	ctrl.vertex[2] = string::convert<float>(_coords["z"].value->GetValue().ToStdString());

	ctrl.texcoord[0] = string::convert<float>(_coords["s"].value->GetValue().ToStdString());
	ctrl.texcoord[1] = string::convert<float>(_coords["t"].value->GetValue().ToStdString());

	patch->getPatchInternal().controlPointsChanged();

	GlobalMainFrame().updateAllWindows();
}

void PatchInspector::emitTesselation()
{
	UndoableCommand setFixedTessCmd("patchSetFixedTesselation");

	wxSpinCtrl* fixedSubdivX = findNamedObject<wxSpinCtrl>(this, "PatchInspectorSubdivisionsX");
	wxSpinCtrl* fixedSubdivY = findNamedObject<wxSpinCtrl>(this, "PatchInspectorSubdivisionsY");

	Subdivisions tess(
		fixedSubdivX->GetValue(),
		fixedSubdivY->GetValue()
	);

	bool fixed = findNamedObject<wxCheckBox>(this, "PatchInspectorFixedSubdivisions")->GetValue();

	// Save the setting into the selected patch(es)
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch)
	{
		patch.setFixedSubdivisions(fixed, tess);
	});

	fixedSubdivX->Enable(fixed);
	fixedSubdivY->Enable(fixed);
	findNamedObject<wxStaticText>(this, "PatchInspectorSubdivisionsXLabel")->Enable(fixed);
	findNamedObject<wxStaticText>(this, "PatchInspectorSubdivisionsYLabel")->Enable(fixed);

	GlobalMainFrame().updateAllWindows();
}

void PatchInspector::onCoordChange(wxCommandEvent& ev)
{
	if (_updateActive) return;

	emitCoords();
}

void PatchInspector::onTessChange(wxSpinEvent& ev)
{
	if (_updateActive) return;

	emitTesselation();
}

void PatchInspector::onFixedTessChange(wxCommandEvent& ev)
{
	if (_updateActive) return;

	emitTesselation();
}

void PatchInspector::onComboBoxChange()
{
	if (_updateActive) return;

	// Load the according patch row/col vertex
	loadControlVertex();
}

void PatchInspector::onClickLarger(CoordRow& row)
{
	// Get the current value and the step increment
	float value = string::convert<float>(row.value->GetValue().ToStdString());
	float step = string::convert<float>(row.stepEntry->GetValue().ToStdString());

	// This triggers the onCoordChange callback method
	row.value->SetValue(string::to_string(value + step));
}

void PatchInspector::onClickSmaller(CoordRow& row)
{
	// Get the current value and the step increment
	float value = string::convert<float>(row.value->GetValue().ToStdString());
	float step = string::convert<float>(row.stepEntry->GetValue().ToStdString());

	// This triggers the onCoordChange callback method
	row.value->SetValue(string::to_string(value - step));
}

// static command target
void PatchInspector::toggle(const cmd::ArgumentList& args)
{
	Instance().ToggleVisibility();
}

void PatchInspector::onPatchControlPointsChanged()
{
	queueUpdate();
}

void PatchInspector::onPatchTextureChanged()
{
	queueUpdate();
}

void PatchInspector::onPatchDestruction()
{
	rescanSelection();
}

void PatchInspector::onIdle(wxIdleEvent& ev)
{
	if (_updateNeeded)
	{
		update();
	}
}

} // namespace ui
