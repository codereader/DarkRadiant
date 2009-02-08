#include "WidgetToggle.h"

WidgetToggle::WidgetToggle() :
	Toggle(MemberCaller<WidgetToggle, &WidgetToggle::doNothing>(*this))
{}
	
	// Dummy callback for the Toggle base class, we don't need any callbacks...
void WidgetToggle::doNothing() {}

/* This method only adds the widget to the show/hide list if the widget
 * is NOT of type GtkCheckMenuItem/GtkToggleToolButtons. Any other
 * widgets are added to the show/hide list */
void WidgetToggle::connectWidget(GtkWidget* widget) {
	// Call the base class method to connect GtkCheckMenuItems and GtkToggleButtons
	Toggle::connectWidget(widget);
	
	// Any other widgets are added to the list
	if (!GTK_IS_CHECK_MENU_ITEM(widget) && !GTK_IS_TOGGLE_TOOL_BUTTON(widget) && widget != NULL) {
		// No special widget, add it to the list
		_widgets.push_back(widget);

		updateWidgets();
	}
}

void WidgetToggle::disconnectWidget(GtkWidget* widget) { 
	// Call the base class method to handle GtkCheckMenuItems and GtkToggleButtons
	Toggle::disconnectWidget(widget);

	WidgetList::iterator i = std::find(_widgets.begin(), _widgets.end(), widget);

	if (i != _widgets.end()) {
		_widgets.erase(i);
	}
}

void WidgetToggle::updateWidgets() {
	// Show/hide the widgets according to the _toggled state
	if (_toggled) {
		showWidgets();
	}
	else {
		hideWidgets();
	}
	
	// Pass the call to the base class to do the rest
	Toggle::updateWidgets();
}
	
// Show all the connected widgets
void WidgetToggle::showWidgets() {
	for (WidgetList::iterator i = _widgets.begin(); i != _widgets.end(); /* in-loop */) {
		if (GTK_IS_WIDGET(*i)) {
			gtk_widget_show(*i++);
		}
		else {
			// Not a valid widget anymore, remove from the list
			_widgets.erase(i++);
		}
	}
}

// Hide all the connected widgets
void WidgetToggle::hideWidgets() {
	for (WidgetList::iterator i = _widgets.begin(); i != _widgets.end(); /* in-loop */) {
		if (GTK_IS_WIDGET(*i)) {
			gtk_widget_hide(*i++);
		}
		else {
			// Not a valid widget anymore, remove from the list
			_widgets.erase(i++);
		}
	}
}
