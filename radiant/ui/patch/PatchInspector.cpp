#include "PatchInspector.h"

#include "i18n.h"
#include "ieventmanager.h"
#include "itextstream.h"
#include "iuimanager.h"
#include "imainframe.h"

#include <gtkmm.h>

#include "registry/bind.h"
#include "selectionlib.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/ControlButton.h"

#include "patch/PatchNode.h"
#include "selection/algorithm/Primitives.h"

namespace ui
{
	namespace
	{
		const char* const WINDOW_TITLE = N_("Patch Inspector");
		const char* const LABEL_CONTROL_VERTICES = N_("Patch Control Vertices");
		const char* const LABEL_COORDS = N_("Coordinates");
		const char* const LABEL_ROW = N_("Row:");
		const char* const LABEL_COL = N_("Column:");
		const char* const LABEL_TESSELATION = N_("Patch Tesselation");
		const char* const LABEL_FIXED = N_("Fixed Subdivisions");
		const char* const LABEL_SUBDIVISION_X = N_("Horizontal:");
		const char* const LABEL_SUBDIVISION_Y = N_("Vertical:");
		const char* const LABEL_STEP = N_("Step:");

		const float TESS_MIN = 1.0f;
		const float TESS_MAX = 32.0f;

		const std::string RKEY_ROOT = "user/ui/patch/patchInspector/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
		const std::string RKEY_X_STEP = RKEY_ROOT + "xCoordStep";
		const std::string RKEY_Y_STEP = RKEY_ROOT + "yCoordStep";
		const std::string RKEY_Z_STEP = RKEY_ROOT + "zCoordStep";
		const std::string RKEY_S_STEP = RKEY_ROOT + "sCoordStep";
		const std::string RKEY_T_STEP = RKEY_ROOT + "tCoordStep";
	}

PatchInspector::PatchInspector()
: gtkutil::PersistentTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow(), true),
  _selectionInfo(GlobalSelectionSystem().getSelectionInfo()),
  _patchRows(0),
  _patchCols(0),
  _updateActive(false)
{
	// Set the default border width in accordance to the HIG
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Create all the widgets and pack them into the window
	populateWindow();

	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(this);

	// Update the widget status
	rescanSelection();

	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);

	_windowPosition.connect(this);
	_windowPosition.applyPosition();
}

PatchInspectorPtr& PatchInspector::InstancePtr()
{
	static PatchInspectorPtr _instancePtr;
	return _instancePtr;
}

void PatchInspector::onRadiantShutdown()
{
	rMessage() << "PatchInspector shutting down." << std::endl;

	if (is_visible())
	{
		hide();
	}

	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);

	GlobalSelectionSystem().removeObserver(this);
	GlobalEventManager().disconnectDialogWindow(this);

	// Destroy the transient window
	destroy();

	// Finally, reset our shared_ptr to destroy the instance
	InstancePtr().reset();
}

void PatchInspector::postUndo()
{
	// Update the PatchInspector after an undo operation
	update();
}

void PatchInspector::postRedo()
{
	// Update the PatchInspector after a redo operation
	update();
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
	// Create the overall vbox
	Gtk::VBox* dialogVBox = Gtk::manage(new Gtk::VBox(false, 6));
	add(*dialogVBox);

	// Create the title label (bold font)
	_vertexChooser.title = Gtk::manage(new gtkutil::LeftAlignedLabel(
    	std::string("<span weight=\"bold\">") + _(LABEL_CONTROL_VERTICES) + "</span>"
    ));
	dialogVBox->pack_start(*_vertexChooser.title, false, false, 0);

    // Setup the table with default spacings
	_vertexChooser.table = Gtk::manage(new Gtk::Table(2, 2, false));
    _vertexChooser.table->set_col_spacings(12);
    _vertexChooser.table->set_row_spacings(6);

    // Pack it into an alignment so that it is indented
	Gtk::Alignment* alignment = Gtk::manage(new gtkutil::LeftAlignment(*_vertexChooser.table, 18, 1.0));
	dialogVBox->pack_start(*alignment, false, false, 0);

	// The vertex col and row chooser
	_vertexChooser.rowLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_ROW)));
	_vertexChooser.table->attach(*_vertexChooser.rowLabel, 0, 1, 0, 1);

	_vertexChooser.rowCombo = Gtk::manage(new Gtk::ComboBoxText);
	_vertexChooser.rowCombo->set_size_request(100, -1);
	_vertexChooser.rowCombo->signal_changed().connect(sigc::mem_fun(*this, &PatchInspector::onComboBoxChange));
	_vertexChooser.table->attach(*_vertexChooser.rowCombo, 1, 2, 0, 1);

	_vertexChooser.colLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_COL)));
	_vertexChooser.table->attach(*_vertexChooser.colLabel, 0, 1, 1, 2);

	_vertexChooser.colCombo = Gtk::manage(new Gtk::ComboBoxText);
	_vertexChooser.colCombo->set_size_request(100, -1);
	_vertexChooser.colCombo->signal_changed().connect(sigc::mem_fun(*this, &PatchInspector::onComboBoxChange));
	_vertexChooser.table->attach(*_vertexChooser.colCombo, 1, 2, 1, 2);

	// Create the title label (bold font)
	_coordsLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(
    	std::string("<span weight=\"bold\">") + _(LABEL_COORDS) + "</span>"
    ));
    _coordsLabel->set_padding(0, 2);
	dialogVBox->pack_start(*_coordsLabel, false, false, 0);

	// Setup the table with default spacings
	_coordsTable = Gtk::manage(new Gtk::Table(5, 2, false));
    _coordsTable->set_col_spacings(12);
    _coordsTable->set_row_spacings(6);

    // Pack it into an alignment so that it is indented
	Gtk::Alignment* coordAlignment = Gtk::manage(new gtkutil::LeftAlignment(*_coordsTable, 18, 1.0));
	dialogVBox->pack_start(*coordAlignment, false, false, 0);

    _coords["x"] = createCoordRow("X:", *_coordsTable, 0);
    _coords["y"] = createCoordRow("Y:", *_coordsTable, 1);
    _coords["z"] = createCoordRow("Z:", *_coordsTable, 2);
    _coords["s"] = createCoordRow("S:", *_coordsTable, 3);
    _coords["t"] = createCoordRow("T:", *_coordsTable, 4);

    // Connect the step values to the according registry values
	using namespace gtkutil;

    registry::bindPropertyToKey(_coords["x"].stepEntry->property_text(),
                                RKEY_X_STEP);
    registry::bindPropertyToKey(_coords["y"].stepEntry->property_text(),
                                RKEY_Y_STEP);
    registry::bindPropertyToKey(_coords["z"].stepEntry->property_text(),
                                RKEY_Z_STEP);
    registry::bindPropertyToKey(_coords["s"].stepEntry->property_text(),
                                RKEY_S_STEP);
    registry::bindPropertyToKey(_coords["t"].stepEntry->property_text(),
                                RKEY_T_STEP);

    // Connect all the arrow buttons
    for (CoordMap::iterator i = _coords.begin(); i != _coords.end(); ++i)
	{
    	CoordRow& row = i->second;

    	// Pass a CoordRow ref to the callback, that's all it will need to update
		row.smaller->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &PatchInspector::onClickSmaller), &row));
		row.larger->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &PatchInspector::onClickLarger), &row));
    }

    // Create the title label (bold font)
	_tesselation.title = Gtk::manage(new gtkutil::LeftAlignedLabel(
    	std::string("<span weight=\"bold\">") + _(LABEL_TESSELATION) + "</span>"
    ));
    _tesselation.title->set_padding(0, 2);
	dialogVBox->pack_start(*_tesselation.title, false, false, 0);

    // Setup the table with default spacings
	_tesselation.table = Gtk::manage(new Gtk::Table(3, 2, false));
    _tesselation.table->set_col_spacings(12);
    _tesselation.table->set_row_spacings(6);

    // Pack it into an alignment so that it is indented
	Gtk::Alignment* tessAlignment = Gtk::manage(new gtkutil::LeftAlignment(*_tesselation.table, 18, 1.0));
	dialogVBox->pack_start(*tessAlignment, false, false, 0);

	// Tesselation checkbox
	_tesselation.fixed = Gtk::manage(new Gtk::CheckButton(_(LABEL_FIXED)));
	_tesselation.fixed->signal_toggled().connect(sigc::mem_fun(*this, &PatchInspector::onFixedTessChange));
	_tesselation.table->attach(*_tesselation.fixed, 0, 2, 0, 1);

	// Tesselation entry fields
	_tesselation.horiz = Gtk::manage(new Gtk::SpinButton);
	_tesselation.vert = Gtk::manage(new Gtk::SpinButton);

	_tesselation.horiz->set_range(TESS_MIN, TESS_MAX);
	_tesselation.vert->set_range(TESS_MIN, TESS_MAX);

	_tesselation.horiz->set_increments(1.0f, 2.0f);
	_tesselation.vert->set_increments(1.0f, 2.0f);

	_tesselation.horiz->set_digits(0);
	_tesselation.vert->set_digits(0);

	_tesselation.horiz->signal_value_changed().connect(sigc::mem_fun(*this, &PatchInspector::onTessChange));
	_tesselation.vert->signal_value_changed().connect(sigc::mem_fun(*this, &PatchInspector::onTessChange));

	_tesselation.horiz->set_size_request(100, -1);
	_tesselation.vert->set_size_request(100, -1);

	_tesselation.horizLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_SUBDIVISION_X)));
	_tesselation.vertLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_SUBDIVISION_Y)));

	_tesselation.table->attach(*_tesselation.horizLabel, 0, 1, 1, 2);
	_tesselation.table->attach(*_tesselation.horiz, 1, 2, 1, 2);

	_tesselation.table->attach(*_tesselation.vertLabel, 0, 1, 2, 3);
	_tesselation.table->attach(*_tesselation.vert, 1, 2, 2, 3);
}

PatchInspector::CoordRow PatchInspector::createCoordRow(
	const std::string& label, Gtk::Table& table, int row)
{
	CoordRow coordRow;

	coordRow.hbox = Gtk::manage(new Gtk::HBox(false, 6));

	// Create the label
	coordRow.label = Gtk::manage(new gtkutil::LeftAlignedLabel(label));
	table.attach(*coordRow.label, 0, 1, row, row + 1);

	// Create the entry field
	coordRow.value = Gtk::manage(new Gtk::Entry);
	coordRow.value->set_width_chars(7);
	coordRow.value->signal_changed().connect(sigc::mem_fun(*this, &PatchInspector::onCoordChange));

	coordRow.hbox->pack_start(*coordRow.value, true, true, 0);

	{
		Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(true, 0));

		coordRow.smaller = Gtk::manage(
			new gtkutil::ControlButton(GlobalUIManager().getLocalPixbuf("arrow_left.png"))
		);
		coordRow.smaller->set_size_request(15, 24);
		hbox->pack_start(*coordRow.smaller, false, false, 0);

		coordRow.larger = Gtk::manage(
			new gtkutil::ControlButton(GlobalUIManager().getLocalPixbuf("arrow_right.png"))
		);
		coordRow.larger->set_size_request(15, 24);
		hbox->pack_start(*coordRow.larger, false, false, 0);

		coordRow.hbox->pack_start(*hbox, false, false, 0);
	}

	// Create the label
	coordRow.steplabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_STEP)));
	coordRow.hbox->pack_start(*coordRow.steplabel, false, false, 0);

	// Create the entry field
	coordRow.stepEntry = Gtk::manage(new Gtk::Entry);
	coordRow.stepEntry->set_width_chars(5);

	coordRow.hbox->pack_start(*coordRow.stepEntry, false, false, 0);

	// Pack the hbox into the table
	table.attach(*coordRow.hbox, 1, 2, row, row + 1);

	// Return the filled structure
	return coordRow;
}

void PatchInspector::onGtkIdle()
{
	// Perform the pending update
	update();
}

void PatchInspector::queueUpdate()
{
	// Request an idle callback to perform the update when GTK is idle
	requestIdleCallback();
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
		int row = string::convert<int>(_vertexChooser.rowCombo->get_active_text());
		int col = string::convert<int>(_vertexChooser.colCombo->get_active_text());

		// Retrieve the controlvertex
		const PatchControl& ctrl = patch->getPatch().ctrlAt(row, col);

		_updateActive = true;

		_coords["x"].value->set_text(string::to_string(ctrl.vertex[0]));
		_coords["y"].value->set_text(string::to_string(ctrl.vertex[1]));
		_coords["z"].value->set_text(string::to_string(ctrl.vertex[2]));
		_coords["s"].value->set_text(string::to_string(ctrl.texcoord[0]));
		_coords["t"].value->set_text(string::to_string(ctrl.texcoord[1]));

		_updateActive = false;
	}
}

// Pre-hide callback
void PatchInspector::_preHide()
{
	// Clear the patch, we don't need to observe it while hidden
	setPatch(PatchNodePtr());

	// A hidden PatchInspector doesn't need to listen for events
	GlobalUndoSystem().removeObserver(this);
	GlobalSelectionSystem().removeObserver(this);

	// Save the window position, to make sure
	_windowPosition.readPosition();
}

// Pre-show callback
void PatchInspector::_preShow()
{
	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);
	GlobalUndoSystem().addObserver(this);

	// Restore the position
	_windowPosition.applyPosition();

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
	_vertexChooser.rowCombo->clear_items();
	_vertexChooser.colCombo->clear_items();

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

	_vertexChooser.table->set_sensitive(sensitive);
	_vertexChooser.title->set_sensitive(sensitive);

	// Tesselation is always sensitive when one or more patches are selected
	_tesselation.title->set_sensitive(_selectionInfo.patchCount > 0);
	_tesselation.table->set_sensitive(_selectionInfo.patchCount > 0);

	_coordsLabel->set_sensitive(sensitive);
	_coordsTable->set_sensitive(sensitive);

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
				tessIsFixed = p.subdivionsFixed();
				tess = p.getSubdivisions();
			}
			else
			{
				// We already have a pair of divisions, compare
				Subdivisions otherTess = p.getSubdivisions();

				if (tessIsFixed != p.subdivionsFixed() || otherTess != tess)
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
		_tesselation.fixed->set_active(tessIsFixed);

		_tesselation.horiz->set_value(tess[0]);
		_tesselation.vert->set_value(tess[1]);

		_tesselation.horiz->set_sensitive(tessIsFixed);
		_tesselation.vert->set_sensitive(tessIsFixed);
		_tesselation.vertLabel->set_sensitive(tessIsFixed);
		_tesselation.horizLabel->set_sensitive(tessIsFixed);

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
		_vertexChooser.rowCombo->append_text(string::to_string(i));
	}

	_vertexChooser.rowCombo->set_active(0);

	for (std::size_t i = 0; i < _patchCols; ++i)
	{
		_vertexChooser.colCombo->append_text(string::to_string(i));
	}

	_vertexChooser.colCombo->set_active(0);

	_updateActive = false;
}

void PatchInspector::emitCoords()
{
	PatchNodePtr patch = _patch.lock();

	if (patch == NULL) return;

	// Save the coords into the patch
	UndoableCommand emitCoordsCmd("patchAdjustControlVertex");

	patch->getPatchInternal().undoSave();

	int row = string::convert<int>(_vertexChooser.rowCombo->get_active_text());
	int col = string::convert<int>(_vertexChooser.colCombo->get_active_text());

	// Retrieve the controlvertex
	PatchControl& ctrl = patch->getPatchInternal().ctrlAt(row, col);

	ctrl.vertex[0] = string::convert<float>(_coords["x"].value->get_text());
	ctrl.vertex[1] = string::convert<float>(_coords["y"].value->get_text());
	ctrl.vertex[2] = string::convert<float>(_coords["z"].value->get_text());

	ctrl.texcoord[0] = string::convert<float>(_coords["s"].value->get_text());
	ctrl.texcoord[1] = string::convert<float>(_coords["t"].value->get_text());

	patch->getPatchInternal().controlPointsChanged();

	GlobalMainFrame().updateAllWindows();
}

void PatchInspector::emitTesselation()
{
	UndoableCommand setFixedTessCmd("patchSetFixedTesselation");

	Subdivisions tess(
		_tesselation.horiz->get_value_as_int(),
		_tesselation.vert->get_value_as_int()
	);

	bool fixed = _tesselation.fixed->get_active();

	// Save the setting into the selected patch(es)
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch)
	{
		patch.setFixedSubdivisions(fixed, tess);
	});

	_tesselation.horiz->set_sensitive(fixed);
	_tesselation.vert->set_sensitive(fixed);
	_tesselation.vertLabel->set_sensitive(fixed);
	_tesselation.horizLabel->set_sensitive(fixed);

	GlobalMainFrame().updateAllWindows();
}

void PatchInspector::onCoordChange()
{
	if (_updateActive) return;

	emitCoords();
}

void PatchInspector::onTessChange()
{
	if (_updateActive) return;

	emitTesselation();
}

void PatchInspector::onFixedTessChange()
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

void PatchInspector::onClickLarger(CoordRow* row)
{
	// Get the current value and the step increment
	float value = string::convert<float>(row->value->get_text());
	float step = string::convert<float>(row->stepEntry->get_text());

	// This triggers the onCoordChange callback method
	row->value->set_text(string::to_string(value + step));
}

void PatchInspector::onClickSmaller(CoordRow* row)
{
	// Get the current value and the step increment
	float value = string::convert<float>(row->value->get_text());
	float step = string::convert<float>(row->stepEntry->get_text());

	// This triggers the onCoordChange callback method
	row->value->set_text(string::to_string(value - step));
}

// static command target
void PatchInspector::toggle(const cmd::ArgumentList& args)
{
	Instance().toggleVisibility();
}

void PatchInspector::onPatchControlPointsChanged()
{
	update();
}

void PatchInspector::onPatchTextureChanged()
{
	update();
}

void PatchInspector::onPatchDestruction()
{
	rescanSelection();
}

} // namespace ui
