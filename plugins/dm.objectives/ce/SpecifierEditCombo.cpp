#include "SpecifierEditCombo.h"
#include "specpanel/SpecifierPanelFactory.h"
#include "../util/TwoColumnTextCombo.h"

#include "gtkutil/TreeModel.h"

#include <gtkmm/liststore.h>
#include <gtkmm/combobox.h>

namespace objectives
{

namespace ce
{

// Constructor
SpecifierEditCombo::SpecifierEditCombo(const SpecifierTypeSet& set) :
	Gtk::HBox(false, 6)
{
	// Create the dropdown containing specifier types
	_specifierCombo = Gtk::manage(new objectives::util::TwoColumnTextCombo);

	Glib::RefPtr<Gtk::ListStore> ls =
		Glib::RefPtr<Gtk::ListStore>::cast_static(_specifierCombo->get_model());

	for (SpecifierTypeSet::const_iterator i = set.begin();
		 i != set.end();
		 ++i)
	{
		Gtk::TreeModel::Row row = *ls->append();

		row.set_value(0, i->getDisplayName());
		row.set_value(1, i->getName());
	}

	_specifierCombo->signal_changed().connect(sigc::mem_fun(*this, &SpecifierEditCombo::_onChange));

	// Main hbox
	pack_start(*_specifierCombo, true, true, 0);

	show_all();
}

// Get the selected Specifier
SpecifierPtr SpecifierEditCombo::getSpecifier() const
{
    return SpecifierPtr(new Specifier(
        SpecifierType::getSpecifierType(getSpecName()),
        _specPanel ? _specPanel->getValue() : ""
    ));
}

// Set the selected Specifier
void SpecifierEditCombo::setSpecifier(SpecifierPtr spec)
{
    // If the SpecifierPtr is null (because the Component object does not have a
    // specifier for this slot), create a default None specifier.
    if (!spec)
        spec = SpecifierPtr(new Specifier());

	// I copied and pasted this from the StimResponseEditor, the SelectionFinder
	// could be cleaned up a bit.
	gtkutil::TreeModel::SelectionFinder finder(spec->getType().getName(), 1);

	_specifierCombo->get_model()->foreach_iter(
		sigc::mem_fun(finder, &gtkutil::TreeModel::SelectionFinder::forEach));

    // SpecifierType name should be found in list
    // Get an iter and set the selected item
	_specifierCombo->set_active(finder.getIter());

    // Create the necessary SpecifierPanel, and set it to display the current
    // value
    createSpecifierPanel(spec->getType().getName());

    if (_specPanel)
	{
        _specPanel->setValue(spec->getValue());
	}
}

// Get the selected SpecifierType string
std::string SpecifierEditCombo::getSpecName() const
{
	// Get the current selection
	Gtk::TreeModel::iterator iter = _specifierCombo->get_active();

	if (iter)
	{
		std::string rv;
		iter->get_value(1, rv);

		return rv;
	}
	else
	{
		return "";
	}
}

// Create the required SpecifierPanel
void SpecifierEditCombo::createSpecifierPanel(const std::string& type)
{
    // Get a panel from the factory
    _specPanel = SpecifierPanelFactory::create(type);

	// If the panel is valid, get its widget and pack into the hbox
	if (_specPanel)
	{
		pack_end(*_specPanel->getWidget(), true, true, 0);
	}
}

void SpecifierEditCombo::_onChange()
{
    // Change the SpecifierPanel
    createSpecifierPanel(getSpecName());
}

}

}
