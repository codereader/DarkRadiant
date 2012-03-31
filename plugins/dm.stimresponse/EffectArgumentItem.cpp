#include "EffectArgumentItem.h"

#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/TreeModel.h"
#include "string/convert.h"

#include <gtkmm/eventbox.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/entry.h>
#include <gtkmm/entrycompletion.h>
#include <gtkmm/label.h>
#include <gtkmm/combobox.h>
#include <gtkmm/comboboxentry.h>

#include "StimTypes.h"

EffectArgumentItem::EffectArgumentItem(
		ResponseEffect::Argument& arg) :
	_arg(arg)
{
	// Pack the label into a eventbox
	_labelBox = Gtk::manage(new Gtk::EventBox);
	Gtk::Label* label = Gtk::manage(new gtkutil::LeftAlignedLabel(_arg.title + ":"));

	_labelBox->add(*label);
	_labelBox->set_tooltip_text(arg.desc);

	// Pack the description widget into a eventbox
	_descBox = Gtk::manage(new Gtk::EventBox);
	Gtk::Label* descLabel = Gtk::manage(new Gtk::Label);
	descLabel->set_markup("<b>?</b>");
	_descBox->add(*descLabel);

	_descBox->set_tooltip_text(arg.desc);
}

// Retrieve the label widget
Gtk::Widget& EffectArgumentItem::getLabelWidget()
{
	return *_labelBox;
}

Gtk::Widget& EffectArgumentItem::getHelpWidget()
{
	return *_descBox;
}

void EffectArgumentItem::save()
{
	// Save the value to the effect
	_arg.value = getValue();
}

// StringArgument
StringArgument::StringArgument(ResponseEffect::Argument& arg) :
	EffectArgumentItem(arg)
{
	_entry = Gtk::manage(new Gtk::Entry);
	_entry->set_text(arg.value);
}

Gtk::Widget& StringArgument::getEditWidget()
{
	return *_entry;
}

std::string StringArgument::getValue()
{
	return _entry->get_text();
}

// Boolean argument
BooleanArgument::BooleanArgument(ResponseEffect::Argument& arg) :
	 EffectArgumentItem(arg)
{
	_checkButton = Gtk::manage(new Gtk::CheckButton(arg.title));
	_checkButton->set_active(!arg.value.empty());
}

Gtk::Widget& BooleanArgument::getEditWidget()
{
	return *_checkButton;
}

std::string BooleanArgument::getValue()
{
	return _checkButton->get_active() ? "1" : "";
}

// Entity Argument
EntityArgument::EntityArgument(
		ResponseEffect::Argument& arg,
		const Glib::RefPtr<Gtk::ListStore>& entityStore) :
	EffectArgumentItem(arg),
	_entityStore(entityStore)
{
	// Create a combo box entry with the given entity list
	_comboBox = Gtk::manage(new Gtk::ComboBoxEntry(_entityStore));

	// Add completion functionality to the combobox entry
	Glib::RefPtr<Gtk::EntryCompletion> completion = Gtk::EntryCompletion::create();

	completion->set_model(_entityStore);
	completion->set_text_column(0);

	_comboBox->get_entry()->set_completion(completion);

	// Sort the list alphabetically
	_entityStore->set_sort_column(0, Gtk::SORT_ASCENDING);

	// Now select the entity passed in the argument
	// Find the entity using a TreeModel traversor (search the column #0)
	gtkutil::TreeModel::SelectionFinder finder(arg.value, 0);

	_entityStore->foreach_iter(
		sigc::mem_fun(finder, &gtkutil::TreeModel::SelectionFinder::forEach));

	// Select the found treeiter, if the name was found in the liststore
	if (finder.getIter())
	{
		_comboBox->set_active(finder.getIter());
	}
}

std::string EntityArgument::getValue()
{
	return _comboBox->get_active_text();
}

Gtk::Widget& EntityArgument::getEditWidget()
{
	return *_comboBox;
}

// StimType Argument
StimTypeArgument::StimTypeArgument(ResponseEffect::Argument& arg, const StimTypes& stimTypes) :
	EffectArgumentItem(arg),
	_stimTypes(stimTypes)
{
	// Cast the helper class onto a ListStore and create a new treeview
	_comboBox = Gtk::manage(new Gtk::ComboBox(_stimTypes.getListStore()));

	// Add the cellrenderer for the name
	Gtk::CellRendererText* nameRenderer = Gtk::manage(new Gtk::CellRendererText);
	Gtk::CellRendererPixbuf* iconRenderer = Gtk::manage(new Gtk::CellRendererPixbuf);

	_comboBox->pack_start(*iconRenderer, false);
	_comboBox->pack_start(*nameRenderer, true);

	_comboBox->add_attribute(iconRenderer->property_pixbuf(), _stimTypes.getColumns().icon);
	_comboBox->add_attribute(nameRenderer->property_text(), _stimTypes.getColumns().captionPlusID);

	iconRenderer->set_fixed_size(26, -1);

	// Now select the stimtype passed in the argument
	// Find the entity using a TreeModel traversor (search the column #0)
	gtkutil::TreeModel::SelectionFinder finder(string::convert<int>(arg.value), _stimTypes.getColumns().id.index());

	_stimTypes.getListStore()->foreach_iter(
		sigc::mem_fun(finder, &gtkutil::TreeModel::SelectionFinder::forEach));

	// Select the found treeiter, if the name was found in the liststore
	if (finder.getIter())
	{
		_comboBox->set_active(finder.getIter());
	}
}

std::string StimTypeArgument::getValue()
{
	Gtk::TreeModel::iterator iter = _comboBox->get_active();

	if (iter)
	{
		return string::to_string((*iter)[_stimTypes.getColumns().id]);
	}

	return "";
}

Gtk::Widget& StimTypeArgument::getEditWidget()
{
	return *_comboBox;
}
