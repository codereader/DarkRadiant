#include "EffectEditor.h"

#include <gtk/gtk.h>

	namespace {
		const std::string WINDOW_TITLE = "Edit Response Effect";
	}

EffectEditor::EffectEditor(GtkWindow* parent) :
	DialogWindow(WINDOW_TITLE, parent)
{
	gtk_window_set_modal(GTK_WINDOW(_window), TRUE);
	populateWindow();
}

void EffectEditor::populateWindow() {
	// Create the overall vbox
	_dialogVBox = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(_window), _dialogVBox);
	
	
}

void EffectEditor::editEffect(StimResponse& response, const unsigned int effectIndex) {
	gtk_widget_show_all(_window);
}
