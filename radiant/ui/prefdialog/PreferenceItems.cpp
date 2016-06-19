#include "PreferenceItems.h"

#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/combobox.h>
#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include "registry/Widgets.h"
#include "wxutil/PathEntry.h"

namespace ui
{

wxWindow* PreferenceLabel::createWidget(wxWindow* parent)
{
	_labelWidget = new wxStaticText(parent, wxID_ANY, "");
	_labelWidget->SetLabelMarkup(_label);

	return _labelWidget;
}

wxWindow* PreferenceEntry::createWidget(wxWindow* parent)
{
	_entryWidget = new wxTextCtrl(parent, wxID_ANY);

	return _entryWidget;
}

void PreferenceEntry::connectWidgetToKey(registry::Buffer& buffer, sigc::signal<void>& resetSignal)
{
	assert(_entryWidget && !_registryKey.empty());

	int minChars = static_cast<int>(std::max(buffer.get(_registryKey).size(), std::size_t(30)));
	_entryWidget->SetMinClientSize(wxSize(_entryWidget->GetCharWidth() * minChars, -1));

	// Connect the registry key to the newly created input field
	registry::bindWidgetToBufferedKey(_entryWidget, _registryKey, buffer, resetSignal);
}

wxWindow* PreferenceCheckbox::createWidget(wxWindow* parent)
{
	_checkbox = new wxCheckBox(parent, wxID_ANY, _flag);

	return _checkbox;
}

void PreferenceCheckbox::connectWidgetToKey(registry::Buffer& buffer, sigc::signal<void>& resetSignal)
{
	assert(_checkbox && !_registryKey.empty());

	// Connect the registry key to this checkbox
	registry::bindWidgetToBufferedKey(_checkbox, _registryKey, buffer, resetSignal);
}

wxWindow* PreferenceCombobox::createWidget(wxWindow* parent)
{
	_choice = new wxChoice(parent, wxID_ANY);

	// Add all the string values to the combo box
	for (const std::string& value : _values)
	{
		_choice->Append(value);
	}

	return _choice;
}

void PreferenceCombobox::connectWidgetToKey(registry::Buffer& buffer, sigc::signal<void>& resetSignal)
{
	assert(_choice && !_registryKey.empty());

	// Connect the registry key to this combo
	registry::bindWidgetToBufferedKey(_choice, _registryKey, buffer, resetSignal, _storeValueNotIndex);
}

wxWindow* PreferencePathEntry::createWidget(wxWindow* parent)
{
	_entry = new wxutil::PathEntry(parent, _browseDirectories);

	return _entry;
}

void PreferencePathEntry::connectWidgetToKey(registry::Buffer& buffer, sigc::signal<void>& resetSignal)
{
	assert(_entry && !_registryKey.empty());

	// Connect the registry key to the newly created input field
	registry::bindWidgetToBufferedKey(_entry->getEntryWidget(), _registryKey, buffer, resetSignal);

	int minChars = static_cast<int>(std::max(GlobalRegistry().get(_registryKey).size(), std::size_t(30)));

	_entry->getEntryWidget()->SetMinClientSize(
		wxSize(_entry->getEntryWidget()->GetCharWidth() * minChars, -1));

	// Initialize entry
	_entry->setValue(registry::getValue<std::string>(_registryKey));
}

wxWindow* PreferenceSpinner::createWidget(wxWindow* parent)
{
	double fraction = static_cast<double>(_fraction);
	double step = 1.0 / fraction;
	unsigned int digits = 0;

	for (; fraction > 1; fraction /= 10)
	{
		++digits;
	}

	if (digits == 0)
	{
		wxSpinCtrl* spinner = new wxSpinCtrl(parent, wxID_ANY);

		spinner->SetRange(static_cast<int>(_lower), static_cast<int>(_upper));

		_spinner = spinner;
	}
	else
	{
		wxSpinCtrlDouble* spinner = new wxSpinCtrlDouble(parent, wxID_ANY);

		spinner->SetRange(_lower, _upper);
		spinner->SetIncrement(step);

		_spinner = spinner;
	}

	_spinner->SetMinClientSize(wxSize(64, -1));

	return _spinner;
}

void PreferenceSpinner::connectWidgetToKey(registry::Buffer& buffer, sigc::signal<void>& resetSignal)
{
	assert(_spinner && !_registryKey.empty());

	if (dynamic_cast<wxSpinCtrl*>(_spinner) != nullptr)
	{
		// Connect the registry key to the newly created input field
		registry::bindWidgetToBufferedKey(static_cast<wxSpinCtrl*>(_spinner), _registryKey, buffer, resetSignal);
	}
	else if (dynamic_cast<wxSpinCtrlDouble*>(_spinner) != nullptr)
	{
		// Connect the registry key to the newly created input field
		registry::bindWidgetToBufferedKey(static_cast<wxSpinCtrlDouble*>(_spinner), _registryKey, buffer, resetSignal);
	}
	else
	{
		rError() << "PereferenceSpinner: type mismatch, expected member to be a wxSpinCtrl or wxSpinCtrlDouble" << std::endl;
		assert(false);
	}
}

wxWindow* PreferenceSlider::createWidget(wxWindow* parent)
{
	// Since sliders are int only, we need to factor the values to support floats
	int factor = static_cast<int>(1 / _stepIncrement);

	wxPanel* panel = new wxPanel(parent, wxID_ANY);

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	panel->SetSizer(hbox);

	_slider = new wxSlider(panel, wxID_ANY, _value * factor, _lower * factor, _upper * factor);
	_slider->SetPageSize(_pageIncrement * factor);

	// Add a text widget displaying the value
	_valueLabel = new wxStaticText(panel, wxID_ANY, "");
	_slider->Bind(wxEVT_SCROLL_CHANGED, [=](wxScrollEvent& ev)
	{
		_valueLabel->SetLabelText(string::to_string(_slider->GetValue()));
		ev.Skip();
	});

	_slider->Bind(wxEVT_SCROLL_THUMBTRACK, [=](wxScrollEvent& ev)
	{
		_valueLabel->SetLabelText(string::to_string(_slider->GetValue()));
		ev.Skip();
	});

	_valueLabel->SetLabelText(string::to_string(_value));

	hbox->Add(_valueLabel, 0, wxALIGN_CENTER_VERTICAL);
	hbox->Add(_slider, 1, wxEXPAND | wxLEFT, 6);

	return panel;
}

void PreferenceSlider::connectWidgetToKey(registry::Buffer& buffer, sigc::signal<void>& resetSignal)
{
	assert(_slider && !_registryKey.empty());

	// Connect the registry key to this slider
	registry::bindWidgetToBufferedKey(_slider, _registryKey, buffer, resetSignal, _factor);

	// Update the value text now, the wxSlider::SetValue() doesn't trigger the changed signal
	_valueLabel->SetLabelText(string::to_string(_slider->GetValue()));
}

}