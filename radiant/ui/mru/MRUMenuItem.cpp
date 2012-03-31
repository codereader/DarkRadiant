#include "MRUMenuItem.h"

#include <gtkmm/container.h>
#include <gtkmm/label.h>

#include "gtkutil/IConv.h"
#include "string/string.h"

#include "MRU.h"

namespace ui {

// Constructor
MRUMenuItem::MRUMenuItem(const std::string& label, ui::MRU& mru, unsigned int index) :
	_label(label),
	_mru(mru),
	_index(index),
	_widget(NULL)
{}

// Copy Constructor
MRUMenuItem::MRUMenuItem(const ui::MRUMenuItem& other) :
	_label(other._label),
	_mru(other._mru),
	_index(other._index),
	_widget(other._widget)
{}

void MRUMenuItem::setWidget(Gtk::Widget* widget)
{
	_widget = widget;
}

Gtk::Widget* MRUMenuItem::getWidget()
{
	return _widget;
}

void MRUMenuItem::activate(const cmd::ArgumentList& args)
{
	// Only pass the call if the MenuItem is not the empty menu item (with index == 0)
	if (_label != "" && _index > 0)
	{
		// Pass the call back to the main MRU class to do the logistics
		_mru.loadMap(_label);
	}
}

void MRUMenuItem::show()
{
	if (_widget != NULL)
	{
		_widget->set_sensitive(_index > 0);
		_widget->show_all();
	}
}

void MRUMenuItem::hide()
{
	if (_widget != NULL)
	{
		_widget->set_sensitive(false);
		_widget->hide();
	}
}

Gtk::Label* MRUMenuItem::findLabel(Gtk::Widget* widget)
{
	Gtk::Container* c = dynamic_cast<Gtk::Container*>(widget);

	if (c == NULL) return NULL;

	Glib::ListHandle<Gtk::Widget*> children = c->get_children();

	for (Glib::ListHandle<Gtk::Widget*>::const_iterator i = children.begin(); i != children.end(); ++i)
	{
		Gtk::Container* container = dynamic_cast<Gtk::Container*>(*i);

		if (container != NULL)
		{
			// Dive in depth-first
			return findLabel(container);
		}

		// No container, check for label
		Gtk::Label* label = dynamic_cast<Gtk::Label*>(*i);

		if (label != NULL)
		{
			return label;
		}
	}

	return NULL; // nothing found
}

void MRUMenuItem::setLabel(const std::string& label)
{
	// Update the internal storage
	_label = label;

	// Add the number prefix to the widget label
	const std::string widgetLabel = string::to_string(_index) + " - " + gtkutil::IConv::localeToUTF8(_label);

	Gtk::Label* childLabel = findLabel(_widget);

	if (childLabel != NULL)
	{
		childLabel->set_text(widgetLabel);
	}

	// Show or hide the widget according to the actual string content
	if (!_label.empty())
	{
		show();
	}
	else
	{
		hide();
	}
}

std::string MRUMenuItem::getLabel() const
{
	return _label;
}

int MRUMenuItem::getIndex() const
{
	return _index;
}

} // namespace ui
