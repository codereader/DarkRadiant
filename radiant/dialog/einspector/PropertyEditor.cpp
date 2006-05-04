#include "PropertyEditor.h"

#include <iostream>

namespace ui
{

// Instantiate a named PropertyEditor subclass

PropertyEditor* PropertyEditor::create(Entity* entity, const char* name) {}

// Register a derived PropertyEditor subclass into the static map

void PropertyEditor::registerClass(const char* name, const PropertyEditor* editor) {
    std::cout << "PropertyEditor: registering " << name << std::endl;   
}

} // namespace ui
