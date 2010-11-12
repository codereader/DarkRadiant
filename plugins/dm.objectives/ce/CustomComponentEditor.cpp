#include "CustomComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"

#include "i18n.h"

namespace objectives
{

namespace ce
{

namespace {
	const char* const DESCRIPTION = N_(
		"A custom component requires no specifiers,\n"
		"the state of this component is manually controlled \n"
		"(i.e. by scripts or triggers).");
}

// Registration helper, will register this editor in the factory
CustomComponentEditor::RegHelper CustomComponentEditor::regHelper;

// Constructor
CustomComponentEditor::CustomComponentEditor(Component& component) :
	_component(&component)
{
	pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_(DESCRIPTION))), false, false, 0);
}

// Write to component
void CustomComponentEditor::writeToComponent() const
{
	// nothing to save here
}

} // namespace ce

} // namespace objectives
