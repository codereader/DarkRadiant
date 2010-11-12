#include "WidgetToggle.h"
#include <boost/bind.hpp>
#include <gtkmm/widget.h>
#include <gtkmm/checkmenuitem.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/toggletoolbutton.h>

WidgetToggle::WidgetToggle() :
	Toggle(boost::bind(&WidgetToggle::doNothing, this, _1))
{}

/* This method only adds the widget to the show/hide list if the widget
 * is NOT of type GtkCheckMenuItem/GtkToggleToolButtons. Any other
 * widgets are added to the show/hide list */
void WidgetToggle::connectWidget(Gtk::Widget* widget)
{
	// Call the base class method to connect GtkCheckMenuItems and GtkToggleButtons
	Toggle::connectWidget(widget);

	// Any other widgets are added to the list
	if (widget != NULL &&
		dynamic_cast<Gtk::CheckMenuItem*>(widget) == NULL &&
		dynamic_cast<Gtk::ToggleToolButton*>(widget) == NULL &&
		dynamic_cast<Gtk::ToggleButton*>(widget) == NULL)
	{
		// No special widget, add it to the list
		_widgets[widget] = widget->connect_property_changed_with_return(
			"visible",
			sigc::bind(sigc::mem_fun(*this, &WidgetToggle::onVisibilityChange), widget)
		);
	}

	// Read the toggled state from the connected widget
	readToggleStateFromWidgets();

	// Update the "regular" widgets now, like check menu items, etc.
	Toggle::updateWidgets();
}

void WidgetToggle::disconnectWidget(Gtk::Widget* widget)
{
	// Call the base class method to handle GtkCheckMenuItems and GtkToggleButtons
	Toggle::disconnectWidget(widget);

	WidgetMap::iterator i = _widgets.find(widget);

	if (i != _widgets.end())
	{
		i->second.disconnect();

		_widgets.erase(i);
	}
}

void WidgetToggle::readToggleStateFromWidgets()
{
	for (WidgetMap::iterator i = _widgets.begin(); i != _widgets.end(); /* in-loop */)
	{
		_toggled = i->first->is_visible();

		++i;
	}
}

void WidgetToggle::updateWidgets()
{
	// Show/hide the widgets according to the _toggled state
	if (_toggled)
	{
		showWidgets();
	}
	else
	{
		hideWidgets();
	}

	// Pass the call to the base class to do the rest
	Toggle::updateWidgets();
}

// Show all the connected widgets
void WidgetToggle::showWidgets()
{
	for (WidgetMap::iterator i = _widgets.begin(); i != _widgets.end(); ++i)
	{
		i->first->show();
	}
}

// Hide all the connected widgets
void WidgetToggle::hideWidgets()
{
	for (WidgetMap::iterator i = _widgets.begin(); i != _widgets.end(); ++i)
	{
		i->first->hide();
	}
}

void WidgetToggle::visibilityChanged()
{
	// Read the state from the connected widgets
	readToggleStateFromWidgets();

	// Update check menu items etc.
	Toggle::updateWidgets();
}

void WidgetToggle::onVisibilityChange(Gtk::Widget* widget)
{
	if (_callbackActive) return;

	// Confirm that the widget's visibility has actually changed
	if (widget->is_visible() != _toggled)
	{
		// Update self
		visibilityChanged();
	}
}
