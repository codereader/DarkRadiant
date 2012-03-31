#include "TransformDialog.h"

#include "i18n.h"
#include "iuimanager.h"
#include "ieventmanager.h"
#include "imainframe.h"
#include "selectionlib.h"
#include "string/string.h"
#include "registry/bind.h"

#include <gtkmm/box.h>
#include <gtkmm/table.h>
#include <gtkmm/entry.h>

#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/ControlButton.h"

#include "selection/algorithm/Transformation.h"

namespace ui {

	namespace {
		const char* const WINDOW_TITLE = N_("Arbitrary Transformation");
		const char* const LABEL_ROTATION = N_("Rotation");
		const char* const LABEL_SCALE = N_("Scale");

		const char* const LABEL_ROTX = N_("X-Axis Rotate:");
		const char* const LABEL_ROTY = N_("Y-Axis Rotate:");
		const char* const LABEL_ROTZ = N_("Z-Axis Rotate:");

		const char* const LABEL_SCALEX = N_("X-Axis Scale:");
		const char* const LABEL_SCALEY = N_("Y-Axis Scale:");
		const char* const LABEL_SCALEZ = N_("Z-Axis Scale:");

		const char* const LABEL_STEP = N_("Step:");

		const std::string RKEY_ROOT = "user/ui/transformDialog/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
		const std::string RKEY_ROTX_STEP = RKEY_ROOT + "rotXStep";
		const std::string RKEY_ROTY_STEP = RKEY_ROOT + "rotYStep";
		const std::string RKEY_ROTZ_STEP = RKEY_ROOT + "rotZStep";

		const std::string RKEY_SCALEX_STEP = RKEY_ROOT + "scaleXStep";
		const std::string RKEY_SCALEY_STEP = RKEY_ROOT + "scaleYStep";
		const std::string RKEY_SCALEZ_STEP = RKEY_ROOT + "scaleZStep";
	}

TransformDialog::TransformDialog()
: gtkutil::PersistentTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow(), true),
  _selectionInfo(GlobalSelectionSystem().getSelectionInfo())
{
	// Set the default border width in accordance to the HIG
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Create all the widgets and pack them into the window
	populateWindow();

	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(this);

	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);

	// Update the widget sensitivity
	update();

	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);

	_windowPosition.connect(this);
	_windowPosition.applyPosition();
}

void TransformDialog::onRadiantShutdown() {

	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);

	GlobalSelectionSystem().removeObserver(this);
	GlobalEventManager().disconnectDialogWindow(this);

	// Destroy the dialog
	destroy();

	InstancePtr().reset();
}

TransformDialogPtr& TransformDialog::InstancePtr() {
	static TransformDialogPtr _instancePtr;

	if (_instancePtr == NULL) {
		// Not yet instantiated, do it now
		_instancePtr = TransformDialogPtr(new TransformDialog);

		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().signal_radiantShutdown().connect(
            sigc::mem_fun(*_instancePtr, &TransformDialog::onRadiantShutdown)
        );
	}

	return _instancePtr;
}

TransformDialog& TransformDialog::Instance() {
	return *InstancePtr();
}

// The command target
void TransformDialog::toggle(const cmd::ArgumentList& args)
{
	Instance().toggleVisibility();
}

void TransformDialog::populateWindow()
{
	// Create the overall vbox and add it to the window container
	_dialogVBox = Gtk::manage(new Gtk::VBox(false, 6));
	add(*_dialogVBox);

	// Create the rotation label (bold font)
	_rotateLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(
    	std::string("<span weight=\"bold\">") + _(LABEL_ROTATION) + "</span>"
    ));
	_dialogVBox->pack_start(*_rotateLabel, false, false, 0);

    // Setup the table with default spacings
	_rotateTable = Gtk::manage(new Gtk::Table(3, 2, false));
	_rotateTable->set_col_spacings(12);
	_rotateTable->set_row_spacings(6);

    // Pack it into an alignment so that it is indented
	Gtk::Widget* rotAlignment = Gtk::manage(new gtkutil::LeftAlignment(*_rotateTable, 18, 1.0));
	_dialogVBox->pack_start(*rotAlignment, false, false, 0);

    _entries["rotateX"] = createEntryRow(_(LABEL_ROTX), *_rotateTable, 0, true, 0);
    _entries["rotateY"] = createEntryRow(_(LABEL_ROTY), *_rotateTable, 1, true, 1);
    _entries["rotateZ"] = createEntryRow(_(LABEL_ROTZ), *_rotateTable, 2, true, 2);

    // Create the scale label (bold font)
	_scaleLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(
    	std::string("<span weight=\"bold\">") + _(LABEL_SCALE) + "</span>"
    ));
	_dialogVBox->pack_start(*_scaleLabel, false, false, 0);

    // Setup the table with default spacings
	_scaleTable = Gtk::manage(new Gtk::Table(3, 2, false));
	_scaleTable->set_col_spacings(12);
	_scaleTable->set_row_spacings(6);

    // Pack it into an alignment so that it is indented
	Gtk::Widget* scaleAlignment = Gtk::manage(new gtkutil::LeftAlignment(*_scaleTable, 18, 1.0));
	_dialogVBox->pack_start(*scaleAlignment, false, false, 0);

	_entries["scaleX"] = createEntryRow(_(LABEL_SCALEX), *_scaleTable, 0, false, 0);
    _entries["scaleY"] = createEntryRow(_(LABEL_SCALEY), *_scaleTable, 1, false, 1);
    _entries["scaleZ"] = createEntryRow(_(LABEL_SCALEZ), *_scaleTable, 2, false, 2);

    // Connect the step values to the according registry values
    registry::bindPropertyToKey(_entries["rotateX"].stepEntry->property_text(),
                                RKEY_ROTX_STEP);
    registry::bindPropertyToKey(_entries["rotateY"].stepEntry->property_text(),
                                RKEY_ROTY_STEP);
    registry::bindPropertyToKey(_entries["rotateZ"].stepEntry->property_text(),
                                RKEY_ROTZ_STEP);
    registry::bindPropertyToKey(_entries["scaleX"].stepEntry->property_text(),
                                RKEY_SCALEX_STEP);
    registry::bindPropertyToKey(_entries["scaleY"].stepEntry->property_text(),
                                RKEY_SCALEY_STEP);
    registry::bindPropertyToKey(_entries["scaleZ"].stepEntry->property_text(),
                                RKEY_SCALEZ_STEP);

    // Connect all the arrow buttons
    for (EntryRowMap::iterator i = _entries.begin(); i != _entries.end(); ++i)
	{
    	EntryRow& row = i->second;

		// Pass a EntryRow pointer to the callback, that's all it will need to update
		row.smaller->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &TransformDialog::onClickSmaller), &row));
		row.larger->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &TransformDialog::onClickLarger), &row));
    }
}

TransformDialog::EntryRow TransformDialog::createEntryRow(
	const std::string& label, Gtk::Table& table, int row, bool isRotator, int axis)
{
	EntryRow entryRow;

	entryRow.isRotator = isRotator;
	entryRow.axis = axis;

	// greebo: The rotation direction is reversed for X and Z rotations
	// This has no mathematical meaning, it's just for looking right.
	entryRow.direction = (isRotator && axis != 1) ? -1 : 1;

	// Create the label
	entryRow.label = Gtk::manage(new gtkutil::LeftAlignedLabel(label));
	table.attach(*entryRow.label, 0, 1, row, row + 1);

	entryRow.hbox = Gtk::manage(new Gtk::HBox(false, 6));

	// Create the control buttons (zero spacing hbox)
	{
		Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(true, 0));

		entryRow.smaller = ControlButtonPtr(
			new gtkutil::ControlButton(GlobalUIManager().getLocalPixbuf("arrow_left.png"))
		);
		entryRow.smaller->set_size_request(15, 24);

		hbox->pack_start(*entryRow.smaller, false, false, 0);

		entryRow.larger = ControlButtonPtr(
			new gtkutil::ControlButton(GlobalUIManager().getLocalPixbuf("arrow_right.png"))
		);
		entryRow.larger->set_size_request(15, 24);
		hbox->pack_start(*entryRow.larger, false, false, 0);

		entryRow.hbox->pack_start(*hbox, false, false, 0);
	}

	// Create the label
	entryRow.stepLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_STEP)));

	entryRow.hbox->pack_start(*entryRow.stepLabel, false, false, 0);

	// Create the entry field
	entryRow.stepEntry = Gtk::manage(new Gtk::Entry);
	entryRow.stepEntry->set_width_chars(5);

	entryRow.hbox->pack_start(*entryRow.stepEntry, false, false, 0);

	// Pack the hbox into the table
	table.attach(*entryRow.hbox, 1, 2, row, row + 1);

	// Return the filled structure
	return entryRow;
}

// Pre-hide callback
void TransformDialog::_preHide()
{
	// Save the window position, to make sure
	_windowPosition.readPosition();
}

// Pre-show callback
void TransformDialog::_preShow()
{
	// Restore the position
	_windowPosition.applyPosition();
	// Update the widget values
	update();
}

void TransformDialog::update()
{
	// Check if there is anything selected
	bool rotSensitive = (_selectionInfo.totalCount > 0);
	bool scaleSensitive = (_selectionInfo.totalCount > 0 && _selectionInfo.entityCount == 0);

	_dialogVBox->set_sensitive(rotSensitive || scaleSensitive);

	// set the sensitivity of the scale/rotation widgets
	_rotateLabel->set_sensitive(rotSensitive);
	_rotateTable->set_sensitive(rotSensitive);

	_scaleLabel->set_sensitive(scaleSensitive);
	_scaleTable->set_sensitive(scaleSensitive);
}

void TransformDialog::selectionChanged(const scene::INodePtr& node, bool isComponent)
{
	update();
}

void TransformDialog::onClickLarger(EntryRow* row)
{
	// Get the current step increment
	float step = string::convert<float>(row->stepEntry->get_text());

	// Determine the action
	if (row->isRotator)
	{
		// Do a rotation
		Vector3 eulerXYZ;

		// Store the value into the right axis
		eulerXYZ[row->axis] = step * row->direction;

		// Pass the call to the algorithm functions
		selection::algorithm::rotateSelected(eulerXYZ);
	}
	else
	{
		// Do a scale
		Vector3 scaleXYZ(1,1,1);

		// Store the value into the right axis
		scaleXYZ[row->axis] = step;

		// Pass the call to the algorithm functions
		selection::algorithm::scaleSelected(scaleXYZ);
	}
}

void TransformDialog::onClickSmaller(EntryRow* row)
{
	// Get the current value and the step increment
	float step = string::convert<float>(row->stepEntry->get_text());

	// Determine the action
	if (row->isRotator)
	{
		// Do a rotation
		Vector3 eulerXYZ;

		// Store the value into the right axis
		eulerXYZ[row->axis] = -step * row->direction;

		// Pass the call to the algorithm functions
		selection::algorithm::rotateSelected(eulerXYZ);
	}
	else
	{
		// Do a scale
		Vector3 scaleXYZ(1,1,1);

		// Store the value into the right axis
		scaleXYZ[row->axis] = 1/step;

		// Pass the call to the algorithm functions
		selection::algorithm::scaleSelected(scaleXYZ);
	}
}

} // namespace ui
