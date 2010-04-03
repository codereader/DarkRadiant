#include "Statement.h"

#include "icommandsystem.h"

Statement::Statement(const std::string& statement, bool reactOnKeyUp) :
	_statement(statement),
	_reactOnKeyUp(reactOnKeyUp)
{}

Statement::~Statement() {
	for (WidgetList::iterator i = _connectedWidgets.begin(); i != _connectedWidgets.end(); ++i) {
		if (GTK_IS_WIDGET(i->first)) {
			g_signal_handler_disconnect(i->first, i->second);
		}
	}
}

bool Statement::empty() const {
	return false;
}

// Invoke the registered callback
void Statement::execute() {
	if (_enabled) {
		GlobalCommandSystem().execute(_statement);
	}
}

// Override the derived keyUp method
void Statement::keyUp() {
	if (_reactOnKeyUp) {
		// Execute the Statement on key up event
		execute();
	}
}

// Override the derived keyDown method
void Statement::keyDown() {
	if (!_reactOnKeyUp) {
		// Execute the Statement on key down event
		execute();
	}
}

// Connect the given menuitem or toolbutton to this Statement
void Statement::connectWidget(GtkWidget* widget) {
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

gboolean Statement::onButtonPress(GtkButton* button, Statement* self)
{
	// Execute the Statement
	self->execute();

	return true;
}

gboolean Statement::onToolButtonPress(GtkToolButton* toolButton, Statement* self)
{
	// Execute the Statement
	self->execute();

	return true;
}

gboolean Statement::onMenuItemClicked(GtkMenuItem* menuitem, Statement* self)
{
	// Execute the Statement
	self->execute();

	return true;
}
