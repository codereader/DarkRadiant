#include "KillComponentEditor.h"

#include "gtkutil/LeftAlignment.h"

#include <gtk/gtk.h>

namespace objectives
{

namespace ce
{

// Registration helper
KillComponentEditor::RegHelper KillComponentEditor::regHelper;

// Constructor
KillComponentEditor::KillComponentEditor()
: _component(NULL)
{
	// Info label
	GtkWidget* label = gtk_label_new("Detect when an AI is killed");
	
	// Main vbox
	_widget = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(
		GTK_BOX(_widget), gtkutil::LeftAlignment(label), FALSE, FALSE, 0
	);
}

// Destructor
KillComponentEditor::~KillComponentEditor() {
	if (GTK_IS_WIDGET(_widget))
		gtk_widget_destroy(_widget);
}

// Get the main widget
GtkWidget* KillComponentEditor::getWidget() const
{
	return _widget;
}

// Set component to edit
void KillComponentEditor::setComponent(Component* component)
{
	_component = component;
}

}

}
