#include "PrefPage.h"

#include "i18n.h"
#include "itextstream.h"
#include "stream/textfilestream.h"
#include "registry/buffer.h"
#include "registry/registry.h"

#include <gtkmm/adjustment.h>
#include <gtkmm/alignment.h>
#include <gtkmm/table.h>
#include <gtkmm/box.h>
#include <gtkmm/scale.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/entry.h>
#include <gtkmm/notebook.h>
#include <gtkmm/checkbutton.h>

#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/dialog/MessageBox.h"
#include "gtkutil/PathEntry.h"
#include "gtkutil/SerialisableWidgets.h"

#include <iostream>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/format.hpp>
#include "modulesystem/ApplicationContextImpl.h"

namespace ui {

	namespace {
		typedef std::vector<std::string> StringVector;
	}

PrefPage::PrefPage(const std::string& name,
                   const std::string& parentPath,
                   Gtk::Notebook* notebook)
: _name(name), _path(parentPath), _notebook(notebook)
{
	// If this is not the root item, add a leading slash
	_path += (!_path.empty()) ? "/" : "";
	_path += _name;

	// Create the overall vbox
	_pageWidget = Gtk::manage(new Gtk::VBox(false, 6));
	_pageWidget->set_border_width(12);

	// Create the label
	_titleLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(
		(boost::format("<b>%s Settings</b>") % _name).str()
	));
	_pageWidget->pack_start(*_titleLabel, false, false, 0);

	// Create the VBOX for all the client widgets
	_vbox = Gtk::manage(new Gtk::VBox(false, 6));

	// Create the alignment for the client vbox and pack it
	Gtk::Widget* alignment = Gtk::manage(new gtkutil::LeftAlignment(*_vbox, 18, 1.0));
	_pageWidget->pack_start(*alignment, false, false, 0);

	// Append the whole vbox as new page to the notebook
	_notebook->append_page(*_pageWidget);
}

void PrefPage::setTitle(const std::string& title)
{
	_titleLabel->set_markup(std::string("<b>" + title + "</b>"));
}

std::string PrefPage::getPath() const
{
	return _path;
}

std::string PrefPage::getName() const
{
	return _name;
}

void PrefPage::saveChanges()
{
	_registryBuffer.commitChanges();
}

void PrefPage::discardChanges()
{
	_registryBuffer.clear();

	_resetValuesSignal();
}

Gtk::Widget& PrefPage::getWidget()
{
	return *_pageWidget;
}

void PrefPage::foreachPage(Visitor& visitor)
{
	for (std::size_t i = 0; i < _children.size(); ++i)
	{
		// Visit this instance
		visitor.visit(_children[i]);

		// Pass the visitor recursively
		_children[i]->foreachPage(visitor);
	}
}

void PrefPage::foreachPage(const std::function<void(PrefPage&)>& functor)
{
	std::for_each(_children.begin(), _children.end(), [&] (const PrefPagePtr& page)
	{
		// Visit this instance
		functor(*page);

		// Pass the visitor recursively
		page->foreachPage(functor);
	});
}

Gtk::Widget* PrefPage::appendCheckBox(const std::string& name,
                                    const std::string& flag,
                                    const std::string& registryKey)
{
	// Create a new checkbox with the given caption and display it
	Gtk::CheckButton* check = Gtk::manage(new Gtk::CheckButton(flag));

	// Connect the registry key to this toggle button
    registry::bindPropertyToBufferedKey(check->property_active(), registryKey, _registryBuffer, _resetValuesSignal);

	appendNamedWidget(name, *check);

	return check;
}

void PrefPage::appendSlider(const std::string& name, const std::string& registryKey, bool drawValue,
                            double value, double lower, double upper, double step_increment, double page_increment, double page_size)
{
	// Create a new adjustment with the boundaries <lower> and <upper> and all the increments
	Gtk::Adjustment* adj = Gtk::manage(new Gtk::Adjustment(value, lower, upper, step_increment, page_increment, page_size));

	// Connect the registry key to this adjustment
    registry::bindPropertyToBufferedKey(adj->property_value(), registryKey, _registryBuffer, _resetValuesSignal);

	// scale
	Gtk::Alignment* alignment = Gtk::manage(new Gtk::Alignment(0.0, 0.5, 1.0, 0.0));
	alignment->show();

	Gtk::HScale* scale = Gtk::manage(new Gtk::HScale(*adj));
	scale->set_value_pos(Gtk::POS_LEFT);
	scale->show();

	alignment->add(*scale);

	scale->set_draw_value(drawValue);
	scale->set_digits((step_increment < 1.0f) ? 2 : 0);

	appendNamedWidget(name, *alignment);
}

namespace
{
    void setRegBufferValueFromActiveText(Gtk::ComboBoxText* combo,
                                   const std::string& key,
								   registry::Buffer& registryBuffer)
    {
        registryBuffer.set(key, combo->get_active_text());
    }

	void setActiveTextFromRegValue(Gtk::ComboBoxText* combo,
                                   const std::string& key)
	{
		combo->set_active_text(GlobalRegistry().get(key));
	}
}

void PrefPage::appendCombo(const std::string& name,
                           const std::string& registryKey,
                           const ComboBoxValueList& valueList,
                           bool storeValueNotIndex)
{
	Gtk::Alignment* alignment = Gtk::manage(new Gtk::Alignment(0.0, 0.5, 0.0, 0.0));

    // Create a new combo box of the correct type
    using boost::shared_ptr;
    using namespace gtkutil;

	Gtk::ComboBoxText* combo = Gtk::manage(new Gtk::ComboBoxText);

    // Add all the string values to the combo box
    for (ComboBoxValueList::const_iterator i = valueList.begin();
         i != valueList.end();
         ++i)
    {
		combo->append_text(*i);
    }

	if (storeValueNotIndex)
	{
        // There is no property_active_text() apparently, so we have to connect
        // manually
        combo->set_active_text(_registryBuffer.get(registryKey));

        combo->property_active().signal_changed().connect(
            sigc::bind(
				sigc::ptr_fun(setRegBufferValueFromActiveText), combo, registryKey, sigc::ref(_registryBuffer)
            )
        );

		_resetValuesSignal.connect(
			sigc::bind(sigc::ptr_fun(setActiveTextFromRegValue), combo, registryKey)
		);
	}
	else
	{
        registry::bindPropertyToBufferedKey(combo->property_active(), registryKey, _registryBuffer, _resetValuesSignal);
	}

    // Add it to the container
    alignment->add(*combo);

	// Add the widget to the dialog row
	appendNamedWidget(name, *alignment);
}

Gtk::Widget* PrefPage::appendEntry(const std::string& name, const std::string& registryKey)
{
	Gtk::Alignment* alignment = Gtk::manage(new Gtk::Alignment(0.0, 0.5, 0.0, 0.0));
	alignment->show();

	Gtk::Entry* entry = Gtk::manage(new Gtk::Entry);
	entry->set_width_chars(static_cast<gint>(std::max(GlobalRegistry().get(registryKey).size(), std::size_t(30))));

	alignment->add(*entry);

	// Connect the registry key to the newly created input field
    registry::bindPropertyToBufferedKey(entry->property_text(), registryKey, _registryBuffer, _resetValuesSignal);

	appendNamedWidget(name, *alignment);

	return entry;
}

Gtk::Widget* PrefPage::appendLabel(const std::string& caption)
{
	Gtk::Label* label = Gtk::manage(new Gtk::Label);
	label->set_markup(caption);

	_vbox->pack_start(*label, false, false, 0);

	return label;
}

Gtk::Widget* PrefPage::appendPathEntry(const std::string& name, const std::string& registryKey, bool browseDirectories)
{
	gtkutil::PathEntry* entry = Gtk::manage(new gtkutil::PathEntry(browseDirectories));

	// Connect the registry key to the newly created input field
    registry::bindPropertyToBufferedKey(entry->getEntryWidget().property_text(),
                                registryKey, _registryBuffer, _resetValuesSignal);

	// Initialize entry
	entry->setValue(GlobalRegistry().get(registryKey));

	appendNamedWidget(name, *entry);

	return entry;
}

Gtk::SpinButton* PrefPage::createSpinner(double value, double lower, double upper, int fraction)
{
	double step = 1.0 / static_cast<double>(fraction);
	unsigned int digits = 0;

	for (;fraction > 1; fraction /= 10)
	{
		++digits;
	}

	Gtk::Adjustment* adjustment = Gtk::manage(new Gtk::Adjustment(value, lower, upper, step, 10, 0));
	Gtk::SpinButton* spin = Gtk::manage(new Gtk::SpinButton(*adjustment, step, digits));

	spin->show();
	spin->set_size_request(64, -1);

	return spin;
}

Gtk::Widget* PrefPage::appendSpinner(const std::string& name, const std::string& registryKey,
                                   double lower, double upper, int fraction)
{
	// Load the initial value (maybe unnecessary, as the value is loaded upon dialog show)
	float value = registry::getValue<float>(registryKey);

	Gtk::Alignment* alignment = Gtk::manage(new Gtk::Alignment(0.0, 0.5, 0.0, 0.0));
	alignment->show();

	Gtk::SpinButton* spin = createSpinner(value, lower, upper, fraction);
	alignment->add(*spin);

	// Connect the registry key to the newly created input field
	registry::bindPropertyToBufferedKey(spin->property_value(), registryKey, _registryBuffer, _resetValuesSignal);

	appendNamedWidget(name, *alignment);

	return spin;
}

PrefPagePtr PrefPage::createOrFindPage(const std::string& path) {
	// Split the path into parts
	StringVector parts;
	boost::algorithm::split(parts, path, boost::algorithm::is_any_of("/"));

	if (parts.size() == 0) {
		std::cout << "Warning: Could not resolve preference path: " << path << "\n";
		return PrefPagePtr();
	}

	PrefPagePtr child;

	// Try to lookup the page in the child list
	for (std::size_t i = 0; i < _children.size(); ++i)
	{
		if (_children[i]->getName() == parts[0]) {
			child = _children[i];
			break;
		}
	}

	if (child == NULL) {
		// No child found, create a new page and add it to the list
		child = PrefPagePtr(new PrefPage(parts[0], _path, _notebook));
		_children.push_back(child);
	}

	// We now have a child with this name, do we have a leaf?
	if (parts.size() > 1) {
		// We have still more parts, split off the first part
		std::string subPath("");
		for (std::size_t i = 1; i < parts.size(); ++i)
		{
			subPath += (subPath.empty()) ? "" : "/";
			subPath += parts[i];
		}
		// Pass the call to the child
		return child->createOrFindPage(subPath);
	}
	else {
		// We have found a leaf, return the child page
		return child;
	}
}

void PrefPage::appendNamedWidget(const std::string& name, Gtk::Widget& widget)
{
	Gtk::Table* table = Gtk::manage(new Gtk::Table(1, 3, true));

	table->set_col_spacings(4);
	table->set_row_spacings(0);

	table->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(name)),
				  0, 1, 0, 1,
				  Gtk::EXPAND|Gtk::FILL, Gtk::AttachOptions(0));

	table->attach(widget, 1, 3, 0, 1, Gtk::EXPAND|Gtk::FILL, Gtk::AttachOptions(0));

	_vbox->pack_start(*table, false, false, 0);
}

} // namespace ui
