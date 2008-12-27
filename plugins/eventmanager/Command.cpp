#include "Command.h"

Command::Command(const Callback& callback, bool reactOnKeyUp) :
	_callback(callback),
	_reactOnKeyUp(reactOnKeyUp)
{}

Command::~Command() {
	for (WidgetList::iterator i = _connectedWidgets.begin(); i != _connectedWidgets.end(); ++i) {
		g_signal_handler_disconnect(i->first, i->second);
	}
}

bool Command::empty() const {
	return false;
}

// Invoke the registered callback
void Command::execute() {
	if (_enabled) {
		_callback();
	}
}

// Override the derived keyUp method
void Command::keyUp() {
	if (_reactOnKeyUp) {
		// Execute the command on key up event
		execute();
	}
}

// Override the derived keyDown method
void Command::keyDown() {
	if (!_reactOnKeyUp) {
		// Execute the command on key down event
		execute();
	}
}

// Connect the given menuitem or toolbutton to this Command
void Command::connectWidget(GtkWidget* widget) {
	if (GTK_IS_MENU_ITEM(widget)) {
		// Connect the static callback function and pass the pointer to this class
		gulong handler = g_signal_connect(G_OBJECT(widget), "activate", G_CALLBACK(onMenuItemClicked), this);

		_connectedWidgets[widget] = handler;
	}
	else if (GTK_IS_TOOL_BUTTON(widget)) {
		// Connect the static callback function and pass the pointer to this class
		gulong handler = g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(onToolButtonPress), this);

		_connectedWidgets[widget] = handler;
	}
	else if (GTK_IS_BUTTON(widget)) {
		// Connect the static callback function and pass the pointer to this class
		gulong handler = g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(onButtonPress), this);

		_connectedWidgets[widget] = handler;
	}
}

gboolean Command::onButtonPress(GtkButton* button, gpointer data) {
	// Retrieve the right Command object from the user <data>
	Command* self = reinterpret_cast<Command*>(data);

	// Execute the command
	self->execute();

	return true;
}

gboolean Command::onToolButtonPress(GtkToolButton* toolButton, gpointer data) {
	// Retrieve the right Command object from the user <data>
	Command* self = reinterpret_cast<Command*>(data);

	// Execute the command
	self->execute();

	return true;
}

gboolean Command::onMenuItemClicked(GtkMenuItem* menuitem, gpointer data) {
	// Retrieve the right Command object from the user <data>
	Command* self = reinterpret_cast<Command*>(data);

	// Execute the command
	self->execute();

	return true;
}
