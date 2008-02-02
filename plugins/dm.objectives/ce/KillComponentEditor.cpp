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
: _component(&component),
  _targetCombo(Specifier::SET_STANDARD_AI())
{
	// Main vbox
	_widget = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(
		GTK_BOX(_widget), _targetCombo.getWidget(), FALSE, FALSE, 0
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

}

}
