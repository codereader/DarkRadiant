#include "WidgetToggle.h"
#include <functional>

#include <wx/toplevel.h>

namespace ui
{

WidgetToggle::WidgetToggle() :
	Toggle(std::bind(&WidgetToggle::doNothing, this, std::placeholders::_1))
{}

/* This method only adds the widget to the show/hide list if the widget
 * is NOT of type GtkCheckMenuItem/GtkToggleToolButtons. Any other
 * widgets are added to the show/hide list */
void WidgetToggle::connectTopLevelWindow(wxTopLevelWindow* widget)
{
	assert(_widgets.find(widget) == _widgets.end());

	_widgets.insert(widget);

	widget->Connect(wxEVT_SHOW, wxShowEventHandler(WidgetToggle::onVisibilityChange), NULL, this);

	// Read the toggled state from the connected widget
	readToggleStateFromWidgets();

	// Update the "regular" widgets now, like check menu items, etc.
	Toggle::updateWidgets();
}

void WidgetToggle::disconnectTopLevelWindow(wxTopLevelWindow* widget)
{
	Widgets::iterator i = _widgets.find(widget);

	if (i != _widgets.end())
	{
		(*i)->Disconnect(wxEVT_SHOW, wxShowEventHandler(WidgetToggle::onVisibilityChange), NULL, this);

		_widgets.erase(i);
	}
}

void WidgetToggle::readToggleStateFromWidgets()
{
	for (Widgets::iterator i = _widgets.begin(); i != _widgets.end(); /* in-loop */)
	{
		_toggled = (*i)->IsShownOnScreen();

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
	for (Widgets::iterator i = _widgets.begin(); i != _widgets.end(); ++i)
	{
		(*i)->Show();
	}
}

// Hide all the connected widgets
void WidgetToggle::hideWidgets()
{
	for (Widgets::iterator i = _widgets.begin(); i != _widgets.end(); ++i)
	{
		(*i)->Hide();
	}
}

void WidgetToggle::visibilityChanged()
{
	// Read the state from the connected widgets
	readToggleStateFromWidgets();

	// Update check menu items etc.
	Toggle::updateWidgets();
}

void WidgetToggle::onVisibilityChange(wxShowEvent& ev)
{
	if (_callbackActive) return;

	// Confirm that the widget's visibility has actually changed
	wxTopLevelWindow* toplevel = dynamic_cast<wxTopLevelWindow*>(ev.GetEventObject());

	if (toplevel != NULL && toplevel->IsShownOnScreen() != _toggled)
	{
		// Update self
		visibilityChanged();
	}
}

}
