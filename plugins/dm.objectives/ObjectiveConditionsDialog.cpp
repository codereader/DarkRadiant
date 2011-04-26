#include "ObjectiveConditionsDialog.h"

#include "imainframe.h"
#include "iuimanager.h"
#include "i18n.h"

#include <gtkmm/button.h>

namespace objectives
{

namespace
{
	const char* const DIALOG_TITLE = N_("Edit Objective Conditions");
}

ObjectiveConditionsDialog::ObjectiveConditionsDialog(const Glib::RefPtr<Gtk::Window>& parent, 
	ObjectiveEntity& objectiveEnt) :
	gtkutil::BlockingTransientWindow(
        _(DIALOG_TITLE), GlobalMainFrame().getTopLevelWindow()
    ),
    gtkutil::GladeWidgetHolder(
        GlobalUIManager().getGtkBuilderFromFile("ObjectiveConditionsDialog.glade")
    ),
	_objectiveEnt(objectiveEnt)
{
	// Window properties
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

	// Add vbox to dialog
    add(*getGladeWidget<Gtk::Widget>("mainVbox"));
    g_assert(get_child() != NULL);

	// OK and CANCEL actions
	getGladeWidget<Gtk::Button>("cancelButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectiveConditionsDialog::_onCancel)
    );
	getGladeWidget<Gtk::Button>("okButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectiveConditionsDialog::_onOK)
    );

	// TODO

	//ObjectiveConditionListColumns _objConditionColumns;
	//Glib::RefPtr<Gtk::ListStore> _objectiveConditionList;
}

void ObjectiveConditionsDialog::_onCancel()
{
	destroy();
}

void ObjectiveConditionsDialog::_onOK()
{
	// TODO: save();
	destroy();
}

} // namespace
