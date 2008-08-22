#include "MissionLogicDialog.h"
#include "ObjectiveEntity.h"

#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "string/string.h"

#include <gtk/gtk.h>

namespace objectives {

namespace {

	const char* const DIALOG_TITLE = "Edit Mission Logic";

	const std::string STANDARD_LOGIC_DESCR = 
		"This is the standard logic for all difficulty levels";

	const std::string DIFFICULTY_LOGIC_DESCR =
		"These logics override the standard logic for the given difficulty level\n" \
		"if the logic string is non-empty.";
}

// Main constructor
MissionLogicDialog::MissionLogicDialog(GtkWindow* parent, ObjectiveEntity& objectiveEnt) :
	gtkutil::BlockingTransientWindow(DIALOG_TITLE, parent),
	_objectiveEnt(objectiveEnt)
{
	// Create one logic editor for each difficulty level plus the default one
	createLogicEditors();

	// Overall VBox for labels and alignments
	GtkWidget* vbx = gtk_vbox_new(FALSE, 12);
	
	gtk_box_pack_start(GTK_BOX(vbx), gtkutil::LeftAlignedLabel("<b>Default Logic</b>"), FALSE, FALSE, 0);

	// Default Logic
	GtkWidget* defaultVBox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(defaultVBox), gtkutil::LeftAlignedLabel(STANDARD_LOGIC_DESCR), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(defaultVBox), _logicEditors[-1]->getWidget(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), gtkutil::LeftAlignment(defaultVBox, 12, 1.0f) , TRUE, TRUE, 0);
	
	// Now add all difficulty-specific editors
	gtk_box_pack_start(GTK_BOX(vbx), gtkutil::LeftAlignedLabel("<b>Difficulty-specific Logic</b>"), FALSE, FALSE, 0);

	GtkWidget* diffVBox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(diffVBox), gtkutil::LeftAlignedLabel(DIFFICULTY_LOGIC_DESCR), FALSE, FALSE, 0);

	// Iterate over all editors for levels that are greater or equal 0
	for (LogicEditorMap::iterator i = _logicEditors.lower_bound(0);
		 i != _logicEditors.end(); i++)
	{
		gtk_box_pack_start(GTK_BOX(diffVBox), 
			gtkutil::LeftAlignedLabel("<b>Logic for Difficulty Level " + intToStr(i->first) + "</b>"), 
			FALSE, FALSE, 0
		);
		gtk_box_pack_start(GTK_BOX(diffVBox), i->second->getWidget(), TRUE, TRUE, 0);
	}

	gtk_box_pack_start(GTK_BOX(vbx), gtkutil::LeftAlignment(diffVBox, 12, 1.0f) , TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbx), gtk_hseparator_new(), FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
	
	// Populate the logic strings
	populateLogicEditors();

	// Add contents to main window
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_container_add(GTK_CONTAINER(getWindow()), vbx);
}

void MissionLogicDialog::createLogicEditors() {
	// Create the default logic editor
	_logicEditors[-1] = LogicEditorPtr(new LogicEditor);
	
	// TODO: Connect this plugin to the difficulty plugin (which can be optional)
	// to find out how many difficulty levels there are (and what their names are)
	_logicEditors[0] = LogicEditorPtr(new LogicEditor);
	_logicEditors[1] = LogicEditorPtr(new LogicEditor);
	_logicEditors[2] = LogicEditorPtr(new LogicEditor);
}

// Create buttons
GtkWidget* MissionLogicDialog::createButtons() {
	// Create a new homogeneous hbox
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(_onOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(_onCancel), this);
	
	gtk_box_pack_end(GTK_BOX(hbx), okButton, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);

	return gtkutil::RightAlignment(hbx);
}

void MissionLogicDialog::populateLogicEditors() {
	// TODO: Connect this plugin to the difficulty plugin (which can be optional)
	// to find out how many difficulty levels there are
	for (int i = -1; i < 2; i++) {
		LogicPtr logic = _objectiveEnt.getMissionLogic(i);

		// FIXME: Hm, maybe it would be better to pass the Logic object itself to the editor?
		_logicEditors[i]->setSuccessLogicStr(logic->successLogic);
		_logicEditors[i]->setFailureLogicStr(logic->failureLogic);
	}
}

void MissionLogicDialog::save() {
	// TODO: Connect this plugin to the difficulty plugin (which can be optional)
	// to find out how many difficulty levels there are
	for (int i = -1; i < 2; i++) {
		LogicPtr logic = _objectiveEnt.getMissionLogic(i);

		// FIXME: Hm, maybe it would be better to pass the Logic object itself to the editor?
		logic->successLogic = _logicEditors[i]->getSuccessLogicStr();
		logic->failureLogic = _logicEditors[i]->getFailureLogicStr();
	}
}

// GTK CALLBACKS

// Save button
void MissionLogicDialog::_onOK(GtkWidget* w, MissionLogicDialog* self) {
    self->save();
	self->destroy();
}

// Cancel button
void MissionLogicDialog::_onCancel(GtkWidget* w, MissionLogicDialog* self) {
    self->destroy();
}

} // namespace objectives
