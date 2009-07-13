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

void PropertyEditor::setProperty(const std::string& key, const std::string& value)
{
	if (_entity == NULL) return;

	UndoableCommand cmd("setProperty");

	_entity->setKeyValue(key, value);	
}

} // namespace ui
