#include "KillComponentEditor.h"
#include "../Specifier.h"

#include "gtkutil/LeftAlignment.h"

#include <gtk/gtk.h>

namespace objectives
{

namespace ce
{

// Registration helper
KillComponentEditor::RegHelper KillComponentEditor::regHelper;

// Constructor
KillComponentEditor::KillComponentEditor(Component& component)
: _component(&component)
{
	// Create a dropdown with available specifier types
	GtkWidget* combo = gtk_combo_box_new_text();
	gtk_combo_box_append_text(
		GTK_COMBO_BOX(combo), Specifier::SPEC_NONE().getName().c_str()
	);
	gtk_combo_box_append_text(
		GTK_COMBO_BOX(combo), Specifier::SPEC_NAME().getName().c_str()
	);
	gtk_combo_box_append_text(
		GTK_COMBO_BOX(combo), Specifier::SPEC_OVERALL().getName().c_str()
	);
	gtk_combo_box_append_text(
		GTK_COMBO_BOX(combo), Specifier::SPEC_CLASSNAME().getName().c_str()
	);
	gtk_combo_box_append_text(
		GTK_COMBO_BOX(combo), Specifier::SPEC_SPAWNCLASS().getName().c_str()
	);
	gtk_combo_box_append_text(
		GTK_COMBO_BOX(combo), Specifier::SPEC_AI_TYPE().getName().c_str()
	);
	gtk_combo_box_append_text(
		GTK_COMBO_BOX(combo), Specifier::SPEC_AI_TEAM().getName().c_str()
	);
	gtk_combo_box_append_text(
		GTK_COMBO_BOX(combo), Specifier::SPEC_AI_INNOCENCE().getName().c_str()
	);

	
	// Main vbox
	_widget = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(_widget), combo, FALSE, FALSE, 0);
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

}

}
