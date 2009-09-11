#include "AIHeadPropertyEditor.h"

#include <gtk/gtkvbox.h>

namespace ui
{

AIHeadPropertyEditor::AIHeadPropertyEditor() :
	_widget(NULL)
{}

AIHeadPropertyEditor::AIHeadPropertyEditor(Entity* entity, 
	const std::string& key, const std::string& options)
{
	_widget = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 6);

	// Create the dropdown box with available heads
}

IPropertyEditorPtr AIHeadPropertyEditor::createNew(Entity* entity, 
	const std::string& key, const std::string& options)
{
	return IPropertyEditorPtr(new AIHeadPropertyEditor(entity, key, options));
}

} // namespace ui
