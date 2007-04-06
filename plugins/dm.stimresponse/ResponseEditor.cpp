#include "ResponseEditor.h"

#include <gtk/gtk.h>

namespace ui {

ResponseEditor::ResponseEditor(StimTypes& stimTypes) :
	ClassEditor(stimTypes)
{
	populatePage();
}

void ResponseEditor::setEntity(SREntityPtr entity) {
	// Pass the call to the base class
	ClassEditor::setEntity(entity);
	
}

void ResponseEditor::entryChanged(GtkEditable* editable) {
	
}

void ResponseEditor::update() {
	
}

void ResponseEditor::populatePage() {
	// Response effects section
    /*gtk_box_pack_start(GTK_BOX(_pageVBox),
    				   gtkutil::LeftAlignedLabel(LABEL_RESPONSE_EFFECTS),
    				   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_pageVBox), 
					   gtkutil::LeftAlignment(createEffectWidgets(), 18, 1.0),
					   TRUE, TRUE, 0);*/
}

void ResponseEditor::openContextMenu(GtkTreeView* view) {
	
}

void ResponseEditor::removeItem(GtkTreeView* view) {
	
}

void ResponseEditor::selectionChanged() {
	
}

} // namespace ui
