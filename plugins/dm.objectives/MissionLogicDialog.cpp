#include "MissionLogicDialog.h"
#include "ObjectiveEntity.h"

#include "i18n.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "string/string.h"

#include <boost/format.hpp>

#include <gtkmm/box.h>
#include <gtkmm/separator.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>

namespace objectives {

namespace {

	const char* const DIALOG_TITLE = N_("Edit Mission Logic");

	const char* const STANDARD_LOGIC_DESCR =
		N_("This is the standard logic for all difficulty levels");

	const char* const DIFFICULTY_LOGIC_DESCR =
		N_("These logics override the standard logic for the given difficulty level\n"
		   "if the logic string is non-empty.");

	inline std::string makeBold(const std::string& input)
	{
		return "<b>" + input + "</b>";
	}
}

// Main constructor
MissionLogicDialog::MissionLogicDialog(const Glib::RefPtr<Gtk::Window>& parent, ObjectiveEntity& objectiveEnt) :
	gtkutil::BlockingTransientWindow(_(DIALOG_TITLE), parent),
	_objectiveEnt(objectiveEnt)
{
	// Create one logic editor for each difficulty level plus the default one
	createLogicEditors();

	// Overall VBox for labels and alignments
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 12));

	vbx->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Default Logic")))), false, false, 0);

	// Default Logic
	Gtk::VBox* defaultVBox = Gtk::manage(new Gtk::VBox(false, 6));
	defaultVBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_(STANDARD_LOGIC_DESCR))), false, false, 0);
	defaultVBox->pack_start(*_logicEditors[-1], true, true, 0);

	vbx->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*defaultVBox, 12, 1.0f)), true, true, 0);

	// Now add all difficulty-specific editors
	vbx->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("Difficulty-specific Logic"))), false, false, 0);

	Gtk::VBox* diffVBox = Gtk::manage(new Gtk::VBox(false, 6));
	diffVBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_(DIFFICULTY_LOGIC_DESCR))), false, false, 0);

	// Iterate over all editors for levels that are greater or equal 0
	for (LogicEditorMap::iterator i = _logicEditors.lower_bound(0);
		 i != _logicEditors.end(); ++i)
	{
		std::string logicStr = (boost::format(_("Logic for Difficulty Level %d")) % i->first).str();

		diffVBox->pack_start(
			*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(logicStr))),
			false, false, 0
		);

		diffVBox->pack_start(*i->second, true, true, 0);
	}

	vbx->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*diffVBox, 12, 1.0f)), true, true, 0);

	vbx->pack_start(*Gtk::manage(new Gtk::HSeparator), false, false, 0);
	vbx->pack_end(createButtons(), false, false, 0);

	// Populate the logic strings
	populateLogicEditors();

	// Add contents to main window
	set_border_width(12);
	add(*vbx);
}

void MissionLogicDialog::createLogicEditors()
{
	// Create the default logic editor
	_logicEditors[-1] = Gtk::manage(new LogicEditor);

	// TODO: Connect this plugin to the difficulty plugin (which can be optional)
	// to find out how many difficulty levels there are (and what their names are)
	_logicEditors[0] = Gtk::manage(new LogicEditor);
	_logicEditors[1] = Gtk::manage(new LogicEditor);
	_logicEditors[2] = Gtk::manage(new LogicEditor);
}

// Create buttons
Gtk::Widget& MissionLogicDialog::createButtons()
{
	// Create a new homogeneous hbox
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(true, 6));

	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));

	okButton->signal_clicked().connect(sigc::mem_fun(*this, &MissionLogicDialog::_onOK));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &MissionLogicDialog::_onCancel));

	hbx->pack_end(*okButton, true, true, 0);
	hbx->pack_end(*cancelButton, true, true, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*hbx));
}

void MissionLogicDialog::populateLogicEditors()
{
	// TODO: Connect this plugin to the difficulty plugin (which can be optional)
	// to find out how many difficulty levels there are
	for (int i = -1; i <= 2; i++)
	{
		LogicPtr logic = _objectiveEnt.getMissionLogic(i);

		// FIXME: Hm, maybe it would be better to pass the Logic object itself to the editor?
		_logicEditors[i]->setSuccessLogicStr(logic->successLogic);
		_logicEditors[i]->setFailureLogicStr(logic->failureLogic);
	}
}

void MissionLogicDialog::save()
{
	// TODO: Connect this plugin to the difficulty plugin (which can be optional)
	// to find out how many difficulty levels there are
	for (int i = -1; i <= 2; i++)
	{
		LogicPtr logic = _objectiveEnt.getMissionLogic(i);

		// FIXME: Hm, maybe it would be better to pass the Logic object itself to the editor?
		logic->successLogic = _logicEditors[i]->getSuccessLogicStr();
		logic->failureLogic = _logicEditors[i]->getFailureLogicStr();
	}
}

// Save button
void MissionLogicDialog::_onOK()
{
    save();
	destroy();
}

// Cancel button
void MissionLogicDialog::_onCancel()
{
    destroy();
}

} // namespace objectives
