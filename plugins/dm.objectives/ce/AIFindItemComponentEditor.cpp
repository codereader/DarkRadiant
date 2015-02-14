#include "AIFindItemComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "i18n.h"
#include <wx/stattext.h>

namespace objectives
{

namespace ce
{

// Registration helper, will register this editor in the factory
AIFindItemComponentEditor::RegHelper AIFindItemComponentEditor::regHelper;

// Constructor
AIFindItemComponentEditor::AIFindItemComponentEditor(wxWindow* parent, Component& component) :
	ComponentEditorBase(parent),
	_component(&component)
{
	// Main vbox
	wxStaticText* label = new wxStaticText(_panel, wxID_ANY, _("Item:"));
	label->SetFont(label->GetFont().Bold());

	_panel->GetSizer()->Add(label, 0, wxBOTTOM, 6);

	// TODO
}

// Write to component
void AIFindItemComponentEditor::writeToComponent() const
{
    if (!_active) return; // still under construction

    assert(_component);
	// TODO
}

} // namespace ce

} // namespace objectives
