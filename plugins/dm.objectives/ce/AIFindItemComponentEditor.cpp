#include "AIFindItemComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "i18n.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"

#include <gtk/gtk.h>

namespace objectives {

namespace ce {

// Registration helper, will register this editor in the factory
AIFindItemComponentEditor::RegHelper AIFindItemComponentEditor::regHelper;

// Constructor
AIFindItemComponentEditor::AIFindItemComponentEditor(Component& component) :
	_component(&component)
{
	pack_start(
		*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Item:") + "</b>")),
        false, false, 0
    );

	// TODO
}

// Write to component
void AIFindItemComponentEditor::writeToComponent() const
{
    assert(_component);
	// TODO
}

} // namespace ce

} // namespace objectives
