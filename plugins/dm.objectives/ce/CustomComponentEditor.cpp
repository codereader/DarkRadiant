#include "CustomComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "i18n.h"

#include <wx/stattext.h>

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
CustomComponentEditor::CustomComponentEditor(wxWindow* parent, Component& component) :
	ComponentEditorBase(parent),
	_component(&component)
{
	_panel->GetSizer()->Add(new wxStaticText(_panel, wxID_ANY, _(DESCRIPTION)), 0);
}

// Write to component
void CustomComponentEditor::writeToComponent() const
{
    if (!_active) return; // still under construction

	// nothing to save here
}

} // namespace ce

} // namespace objectives
