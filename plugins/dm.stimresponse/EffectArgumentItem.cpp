#include "EffectArgumentItem.h"

#include "string/convert.h"

#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>
#include <wx/bmpcbox.h>

#include "gtkutil/ChoiceHelper.h"

#include "StimTypes.h"

EffectArgumentItem::EffectArgumentItem(wxWindow* parent,
		ResponseEffect::Argument& arg) :
	_arg(arg)
{
	// Create the label
	_label = new wxStaticText(parent, wxID_ANY, _arg.title + ":");
	_label->SetToolTip(arg.desc);

	// Create the description widget
	_descBox = new wxStaticText(parent, wxID_ANY, "?");
	_descBox->SetFont(_descBox->GetFont().Bold());
	_descBox->SetToolTip(arg.desc);
}

// Retrieve the label widget
wxWindow* EffectArgumentItem::getLabelWidget()
{
	return _label;
}

wxWindow* EffectArgumentItem::getHelpWidget()
{
	return _descBox;
}

void EffectArgumentItem::save()
{
	// Save the value to the effect
	_arg.value = getValue();
}

// StringArgument
StringArgument::StringArgument(wxWindow* parent, ResponseEffect::Argument& arg) :
	EffectArgumentItem(parent, arg)
{
	_entry = new wxTextCtrl(parent, wxID_ANY);
	_entry->SetValue(arg.value);
}

wxWindow* StringArgument::getEditWidget()
{
	return _entry;
}

std::string StringArgument::getValue()
{
	return _entry->GetValue().ToStdString();
}

// Boolean argument
BooleanArgument::BooleanArgument(wxWindow* parent, ResponseEffect::Argument& arg) :
	 EffectArgumentItem(parent, arg)
{
	_checkButton = new wxCheckBox(parent, wxID_ANY, arg.title);
	_checkButton->SetValue(!arg.value.empty());
}

wxWindow* BooleanArgument::getEditWidget()
{
	return _checkButton;
}

std::string BooleanArgument::getValue()
{
	return _checkButton->GetValue() ? "1" : "";
}

// Entity Argument
EntityArgument::EntityArgument(
		wxWindow* parent,
		ResponseEffect::Argument& arg,
		const wxArrayString& entityChoices) :
	EffectArgumentItem(parent, arg)
{
	// Create a combo box entry with the given entity list
	_comboBox = new wxComboBox(parent, wxID_ANY);
	_comboBox->Append(entityChoices);

	_comboBox->SetValue(arg.value);
}

std::string EntityArgument::getValue()
{
	return _comboBox->GetValue().ToStdString();
}

wxWindow* EntityArgument::getEditWidget()
{
	return _comboBox;
}

// StimType Argument
StimTypeArgument::StimTypeArgument(wxWindow* parent, 
								   ResponseEffect::Argument& arg, 
								   const StimTypes& stimTypes) :
	EffectArgumentItem(parent, arg),
	_stimTypes(stimTypes)
{
#if USE_BMP_COMBO_BOX
	_comboBox = new wxBitmapComboBox(parent, wxID_ANY);
#else
	_comboBox = new wxComboBox(parent, wxID_ANY);
#endif
	_stimTypes.populateComboBox(_comboBox);

	StimType stimType = _stimTypes.get(string::convert<int>(arg.value));

	wxutil::ChoiceHelper::SelectItemByStoredString(_comboBox, stimType.name);
}

std::string StimTypeArgument::getValue()
{
	// The effect argument stores the stim type ID as value
	if (_comboBox->GetSelection() != wxNOT_FOUND)
	{
		wxClientData* data = _comboBox->GetClientObject(_comboBox->GetSelection());

		if (data == NULL) return "";

		wxStringClientData* nameStr = dynamic_cast<wxStringClientData*>(data);

		int id = _stimTypes.getIdForName(nameStr->GetData().ToStdString());

		if (id == -1) return "";

		return string::to_string(id);
	}

	return "";
}

wxWindow* StimTypeArgument::getEditWidget()
{
	return _comboBox;
}
