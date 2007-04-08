#include "SoundPropertyEditor.h"

namespace ui
{

// Main constructor
SoundPropertyEditor::SoundPropertyEditor(Entity* entity,
									     const std::string& name,
									     const std::string& options)
: PropertyEditor(entity, name)
{

}

// Set the value in the widgets
void SoundPropertyEditor::setValue(const std::string& val) {
	std::cout << "Set value " << val << std::endl;
}

// Return the value in the widgets
const std::string SoundPropertyEditor::getValue() {
	return "None";
}

} // namespace ui
