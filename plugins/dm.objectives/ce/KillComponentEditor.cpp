#include "KillComponentEditor.h"

#include <gtk/gtk.h>

namespace objectives
{

namespace ce
{

// Registration helper
KillComponentEditor::RegistrationHelper KillComponentEditor::registrationHelper;

GtkWidget* KillComponentEditor::getWidget() const
{
	return gtk_label_new("Kill component");
}

}

}
