#include "ColourPropertyEditor.h"

namespace ui
{

// Blank ctor
ColourPropertyEditor::ColourPropertyEditor() {}

// Main ctor
ColourPropertyEditor::ColourPropertyEditor(Entity* entity, const std::string& name)
: PropertyEditor(entity, name, "colour") {
}

// Set displayed value
void ColourPropertyEditor::setValue(const std::string& val) {
}

// Get current value
const std::string ColourPropertyEditor::getValue() {

}
	
} // namespace ui

