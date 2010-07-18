#include "PropertyEditor.h"

#include "ientity.h"
#include "iundo.h"

namespace ui
{

PropertyEditor::PropertyEditor() :
	_entity(NULL)
{}

PropertyEditor::PropertyEditor(Entity* entity) :
	_entity(entity)
{}

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
