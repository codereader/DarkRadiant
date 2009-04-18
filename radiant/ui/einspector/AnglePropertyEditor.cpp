#include "AnglePropertyEditor.h"

#include <gtk/gtk.h>

namespace ui
{

// Constructor
AnglePropertyEditor::AnglePropertyEditor(Entity* entity, const std::string& key)
{
    _widget = gtk_button_new_with_label("Angle");
}

}
