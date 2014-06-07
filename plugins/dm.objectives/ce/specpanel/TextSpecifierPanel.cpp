#include "TextSpecifierPanel.h"

#include <wx/textctrl.h>

namespace objectives
{

namespace ce
{

// Constructor
TextSpecifierPanel::TextSpecifierPanel() :
	_entry(NULL)
{}

TextSpecifierPanel::TextSpecifierPanel(wxWindow* parent) :
	_entry(new wxTextCtrl(parent, wxID_ANY))
{}

wxWindow* TextSpecifierPanel::getWidget()
{
	if (_entry == NULL)
	{
		throw std::runtime_error("Cannot pack a SpecifierPanel created by its default constructor.");
	}

	return _entry;
}

// Set the displayed value
void TextSpecifierPanel::setValue(const std::string& value)
{
	assert(_entry != NULL);
	_entry->SetValue(value);
}

// Get the edited value
std::string TextSpecifierPanel::getValue() const
{
	assert(_entry != NULL);
	return _entry->GetValue().ToStdString();
}

} // namespace ce

} // namespace objectives
