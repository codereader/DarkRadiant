#include "KillComponentEditor.h"

#include <gtk/gtk.h>

namespace objectives
{

namespace ce
{

// Registration helper
KillComponentEditor::RegHelper KillComponentEditor::regHelper;

GtkWidget* KillComponentEditor::getWidget() const
{
	return gtk_label_new("Kill component");
}

void KillComponentEditor::setComponent(Component* component)
{
	_component = component;
}

}

}
