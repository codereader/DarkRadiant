#include "SkinPropertyEditor.h"

namespace ui
{

// Main constructor
SkinPropertyEditor::SkinPropertyEditor(Entity* entity,
									   const std::string& name,
									   const std::string& options)
: PropertyEditor(entity, name)
{
	// Pack in dummy widget
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(getEditWindow()),
										  gtk_label_new("SkinPropertyEditor"));
	
}

// Set the value in the widgets
void SkinPropertyEditor::setValue(const std::string& val) {
	
}

// Return the value in the widgets
const std::string SkinPropertyEditor::getValue() {
	return "none";
}

}
