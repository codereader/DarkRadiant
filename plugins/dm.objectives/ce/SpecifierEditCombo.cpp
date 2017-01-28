#include "SpecifierEditCombo.h"
#include "specpanel/SpecifierPanelFactory.h"

#include <wx/sizer.h>
#include <wx/choice.h>
#include "string/convert.h"
#include "wxutil/ChoiceHelper.h"

namespace objectives
{

namespace ce
{

// Constructor
SpecifierEditCombo::SpecifierEditCombo(wxWindow* parent, 
									   const std::function<void()>& valueChanged, 
									   const SpecifierTypeSet& set) :
	wxPanel(parent, wxID_ANY),
	_valueChanged(valueChanged)
{
	SetSizer(new wxBoxSizer(wxHORIZONTAL));

	// Create the dropdown containing specifier types
	_specifierCombo = new wxChoice(this, wxID_ANY);

	for (SpecifierTypeSet::const_iterator i = set.begin();
		 i != set.end(); ++i)
	{
		_specifierCombo->Append(
			i->getDisplayName(), 
			new wxStringClientData(string::to_string(i->getId())));
	}

	_specifierCombo->Connect(wxEVT_CHOICE, 
		wxCommandEventHandler(SpecifierEditCombo::_onChange), NULL, this);

	GetSizer()->Add(_specifierCombo, 1, wxRIGHT | wxEXPAND, 6);
}

// Get the selected Specifier
SpecifierPtr SpecifierEditCombo::getSpecifier() const
{
    return SpecifierPtr(new Specifier(
        SpecifierType::getSpecifierType(getSpecName()),
        _specPanel ? _specPanel->getValue() : ""
    ));
}

// Set the selected Specifier
void SpecifierEditCombo::setSpecifier(SpecifierPtr spec)
{
    // If the SpecifierPtr is null (because the Component object does not have a
    // specifier for this slot), create a default None specifier.
    if (!spec)
    {
        spec = std::make_shared<Specifier>();
    }

	// I copied and pasted this from the StimResponseEditor, the SelectionFinder
	// could be cleaned up a bit.
	wxutil::ChoiceHelper::SelectItemByStoredId(_specifierCombo, spec->getType().getId());
	
    // Create the necessary SpecifierPanel, and set it to display the current
    // value
    createSpecifierPanel(spec->getType().getName());

    if (_specPanel)
	{
        _specPanel->setValue(spec->getValue());
	}
}

// Get the selected SpecifierType string
std::string SpecifierEditCombo::getSpecName() const
{
	// Get the current selection
	int selectedId = wxutil::ChoiceHelper::GetSelectionId(_specifierCombo);

	if (selectedId != -1)
	{
		SpecifierType specType = SpecifierType::getSpecifierType(selectedId);

		return specType.getName();
	}
	else
	{
		return "";
	}
}

// Create the required SpecifierPanel
void SpecifierEditCombo::createSpecifierPanel(const std::string& type)
{
    // Get a panel from the factory
    _specPanel = SpecifierPanelFactory::create(this, type);

	// If the panel is valid, get its widget and pack into the hbox
	if (_specPanel)
	{
		// Wire up the changed signal, we want to call ComponentEditor::writeToComponent
		// when anything changes
		_specPanel->setChangedCallback(_valueChanged);

		GetSizer()->Add(_specPanel->getWidget(), 1, wxEXPAND);
	}

	// As first measure, we fire the callback itself, as the specifier type changed
	_valueChanged();

	Layout();
}

void SpecifierEditCombo::_onChange(wxCommandEvent& ev)
{
    // Change the SpecifierPanel
    createSpecifierPanel(getSpecName());
}

}

}
