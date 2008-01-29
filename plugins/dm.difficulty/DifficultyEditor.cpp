#include "DifficultyEditor.h"

#include "iradiant.h"
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkimage.h>

namespace ui {

	namespace {
		const std::string DIFF_ICON("sr_icon_custom.png");
	}

DifficultyEditor::DifficultyEditor(const std::string& label, 
								   const difficulty::DifficultySettingsPtr& settings) :
	_settings(settings)
{
	// The tab label items (icon + label)
	_labelHBox = gtk_hbox_new(FALSE, 3);
	_label = gtk_label_new(label.c_str());

	gtk_box_pack_start(
    	GTK_BOX(_labelHBox), 
    	gtk_image_new_from_pixbuf(GlobalRadiant().getLocalPixbufWithMask(DIFF_ICON)), 
    	FALSE, FALSE, 3
    );
	gtk_box_pack_start(GTK_BOX(_labelHBox), _label, FALSE, FALSE, 3);

	// The actual editor pane
	_editor = gtk_hbox_new(FALSE, 3);
}

GtkWidget* DifficultyEditor::getEditor() {
	return _editor;
}

// Returns the label for packing into a GtkNotebook tab.
GtkWidget* DifficultyEditor::getNotebookLabel() {
	return _labelHBox;
}

void DifficultyEditor::setLabel(const std::string& label) {
	gtk_label_set_markup(GTK_LABEL(_label), label.c_str());
}

} // namespace ui
