#include "CommandArgumentItem.h"

#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/TreeModel.h"
#include "string/convert.h"

#include <gtkmm/eventbox.h>
#include <gtkmm/entry.h>
#include <gtkmm/combobox.h>

namespace ui {

CommandArgumentItem::CommandArgumentItem(
		const conversation::ArgumentInfo& argInfo) :
	_argInfo(argInfo)
{
	// Pack the label into an eventbox
	_labelBox = Gtk::manage(new Gtk::EventBox);
	_labelBox->set_tooltip_markup(argInfo.description);

	Gtk::Label* label = Gtk::manage(new gtkutil::LeftAlignedLabel(_argInfo.title + ":"));
	_labelBox->add(*label);

	// Pack the description widget into an eventbox
	_descBox = Gtk::manage(new Gtk::EventBox);
	_descBox->set_tooltip_markup(argInfo.description);

	Gtk::Label* descLabel = Gtk::manage(new Gtk::Label);
	descLabel->set_markup("<b>?</b>");
	_descBox->add(*descLabel);
}

// Retrieve the label widget
Gtk::Widget& CommandArgumentItem::getLabelWidget()
{
	return *_labelBox;
}

Gtk::Widget& CommandArgumentItem::getHelpWidget()
{
	return *_descBox;
}

// StringArgument
StringArgument::StringArgument(
		const conversation::ArgumentInfo& argInfo) :
	CommandArgumentItem(argInfo)
{
	_entry = Gtk::manage(new Gtk::Entry);
	//gtk_entry_set_text(GTK_ENTRY(_entry), argInfo.value.c_str());
}

Gtk::Widget& StringArgument::getEditWidget()
{
	return *_entry;
}

std::string StringArgument::getValue()
{
	return _entry->get_text();
}

void StringArgument::setValueFromString(const std::string& value)
{
	_entry->set_text(value);
}

// Boolean argument
BooleanArgument::BooleanArgument(const conversation::ArgumentInfo& argInfo) :
	 CommandArgumentItem(argInfo)
{
	_checkButton = Gtk::manage(new Gtk::CheckButton(argInfo.title));
	//gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_checkButton), !argInfo.value.empty());
}

Gtk::Widget& BooleanArgument::getEditWidget()
{
	return *_checkButton;
}

std::string BooleanArgument::getValue()
{
	return _checkButton->get_active() ? "1" : "";
}

void BooleanArgument::setValueFromString(const std::string& value)
{
	_checkButton->set_active(value == "1");
}

// Actor Argument
ActorArgument::ActorArgument(
		const conversation::ArgumentInfo& argInfo,
		const Glib::RefPtr<Gtk::ListStore>& actorStore,
		const ActorColumns& actorColumns) :
	CommandArgumentItem(argInfo),
	_actorColumns(actorColumns),
	_actorStore(actorStore)
{
	// Cast the helper class onto a ListStore and create a new treeview
	_comboBox = Gtk::manage(new Gtk::ComboBox(Glib::RefPtr<Gtk::TreeModel>::cast_static(_actorStore)));

	// Add the cellrenderer for the name
	Gtk::CellRendererText* nameRenderer = Gtk::manage(new Gtk::CellRendererText);

	_comboBox->pack_start(*nameRenderer, true);
	_comboBox->add_attribute(nameRenderer->property_text(), _actorColumns.caption);
}

std::string ActorArgument::getValue()
{
	Gtk::TreeModel::const_iterator iter = _comboBox->get_active();

	if (iter)
	{
		Gtk::TreeModel::Row row = *iter;
		return string::to_string(row[_actorColumns.actorNumber]);
	}

	return "";
}

void ActorArgument::setValueFromString(const std::string& value)
{
	// Convert the string to an actor ID
	int actorId = string::convert<int>(value, -1);

	if (actorId == -1) return; // invalid actor id

	// Find the actor id in the liststore
	gtkutil::TreeModel::SelectionFinder finder(actorId, _actorColumns.actorNumber.index());

	_actorStore->foreach_iter(sigc::mem_fun(finder, &gtkutil::TreeModel::SelectionFinder::forEach));

	const Gtk::TreeModel::iterator iter = finder.getIter();

	if (iter)
	{
		_comboBox->set_active(iter);
	}
}

Gtk::Widget& ActorArgument::getEditWidget()
{
	return *_comboBox;
}

} // namespace ui
