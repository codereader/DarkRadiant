#include "Toggle.h"

Toggle::Toggle(const Callback& callback) :
	_callback(callback),
	_toggleToolButton(NULL),
	_checkMenuItem(NULL),
	_callbackActive(false),
	_toggled(false)
{}

bool Toggle::empty() const {
	return false;
}

// Set the toggled state to true/false, according to <toggled> and update
// any associated widgets or notify any callbacks.
bool Toggle::setToggled(const bool toggled) {
	if (_callbackActive) {
		return false;
	}
	
	// Update the toggle status and export it to the GTK button
	_toggled = toggled;
	updateWidgets();
	
	return true;
}

void Toggle::updateWidgets() {
	_callbackActive = true;
	
	if (_toggleToolButton != NULL) {
		gtk_toggle_tool_button_set_active(_toggleToolButton, _toggled);
	}
	
	if (_checkMenuItem != NULL) {
		gtk_check_menu_item_set_active(_checkMenuItem, _toggled);
	}
	
	_callbackActive = false;
}

// On key press >> toggle the internal state
void Toggle::keyDown() {
	toggle();
}

bool Toggle::isToggled() const {
	return _toggled;
}

void Toggle::connectWidget(GtkWidget* widget) {
	if (GTK_IS_TOGGLE_TOOL_BUTTON(widget)) {
		// Store the pointer for later use
		_toggleToolButton = GTK_TOGGLE_TOOL_BUTTON(widget);
	
		gtk_toggle_tool_button_set_active(_toggleToolButton, _toggled);
		
		// Connect the toggleToolbutton to the static callback of this class
		g_signal_connect(G_OBJECT(_toggleToolButton), "toggled", G_CALLBACK(onToggleToolButtonClicked), this);
	}
	else if (GTK_IS_CHECK_MENU_ITEM(widget)) {
		// Store it internally
		_checkMenuItem = GTK_CHECK_MENU_ITEM(widget);
		
		gtk_check_menu_item_set_active(_checkMenuItem, _toggled);
		
		g_signal_connect(G_OBJECT(_checkMenuItem), "toggled", G_CALLBACK(onCheckMenuItemClicked), this);
	}
}

// Invoke the registered callback and update/notify
void Toggle::toggle() {
	if (_callbackActive) {
		return;
	}
	
	// Check if the toggle event is enabled
	if (_enabled) {
		// Invert the <toggled> state 
		_toggled = !_toggled;
		
		// Call the connected function
		_callback();
	}
	
	// Update any attached GtkObjects in any case
	updateWidgets();
}

// The static GTK callback methods that can be connected to a ToolButton or a MenuItem
gboolean Toggle::onToggleToolButtonClicked(GtkToggleToolButton* toolButton, gpointer data) {
	
	// Cast the passed pointer onto a Toggle class. Note: this may also call a toggle() method
	// of a derived class.
	Toggle* self = reinterpret_cast<Toggle*>(data);
	
	// Check sanity and toggle this item
	if (self != NULL) {
		self->toggle();
	}
	
	return true;
}

gboolean Toggle::onCheckMenuItemClicked(GtkMenuItem* menuitem, gpointer data) {
	Toggle* self = reinterpret_cast<Toggle*>(data);
	
	// Check sanity and toggle this item
	if (self != NULL) {
		self->toggle();
	}
	
	return true;
}
