#include "StimEditor.h"

#include <gtk/gtk.h>

namespace ui {

StimEditor::StimEditor() {
	populatePage();
}

StimEditor::operator GtkWidget*() {
	return _pageVBox;
}

void StimEditor::populatePage() {
	_pageVBox = gtk_vbox_new(FALSE, 6);
}

} // namespace ui
