#include "PropertyEditor.h"

#include "ientity.h"
#include "iundo.h"
#include <gtkmm/widget.h>

namespace ui
{

PropertyEditor::PropertyEditor() :
	_mainWidget(NULL),
	_entity(NULL)
{}

PropertyEditor::PropertyEditor(Entity* entity) :
	_mainWidget(NULL),
	_entity(entity)
{}

PropertyEditor::~PropertyEditor()
{
	// Destroy the widget
	delete _mainWidget;
}

void PropertyEditor::setMainWidget(Gtk::Widget* widget)
{
	_mainWidget = widget;
}

Gtk::Widget& PropertyEditor::getWidget()
{
	assert(_mainWidget); // should be set by the subclass at this point
	return *_mainWidget;
}

std::string PropertyEditor::getKeyValue(const std::string& key)
{
	return (_entity != NULL) ? _entity->getKeyValue(key) : "";
}

void PropertyEditor::setKeyValue(const std::string& key, const std::string& value)
{
	if (_entity == NULL) return;

	UndoableCommand cmd("setProperty");

	_entity->setKeyValue(key, value);
}

} // namespace ui
