#include "CustomStimEditor.h"

#include <gtk/gtk.h>

namespace ui {

/** greebo: Constructor creates all the widgets
 */
CustomStimEditor::CustomStimEditor(StimTypes& stimTypes) :
	_stimTypes(stimTypes)
{
	populatePage();
}

CustomStimEditor::operator GtkWidget*() {
	return gtk_vbox_new(FALSE, 3);
}

/** greebo: As the name states, this creates the context menu widgets.
 */
void CustomStimEditor::createContextMenu() {
	
}

/** greebo: Creates all the widgets
 */
void CustomStimEditor::populatePage() {
	
}
	
} // namespace ui
