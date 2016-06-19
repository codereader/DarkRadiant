#include "PreferenceItem.h"

#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/sizer.h>
#include <wx/combobox.h>
#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include "registry/Widgets.h"
#include "wxutil/PathEntry.h"

namespace ui
{

wxWindow* PreferenceItem::createLabel(const std::string& label)
{
	wxStaticText* labelWidget = new wxStaticText(_parent, wxID_ANY, "");
	labelWidget->SetLabelMarkup(label);

	return labelWidget;
}

wxWindow* PreferenceItem::createEntry()
{
	wxTextCtrl* entryWidget = new wxTextCtrl(_parent, wxID_ANY);

	int minChars = static_cast<int>(std::max(_buffer.get(_registryKey).size(), std::size_t(30)));
	entryWidget->SetMinClientSize(wxSize(entryWidget->GetCharWidth() * minChars, -1));

	// Connect the registry key to the newly created input field
	registry::bindWidgetToBufferedKey(entryWidget, _registryKey, _buffer, _resetSignal);

	return entryWidget;
}

wxWindow* PreferenceItem::createCheckbox(const std::string& label)
{
	wxCheckBox* checkbox = new wxCheckBox(_parent, wxID_ANY, label);

	// Connect the registry key to this checkbox
	registry::bindWidgetToBufferedKey(checkbox, _registryKey, _buffer, _resetSignal);

	return checkbox;
}

wxWindow* PreferenceItem::createCombobox(const ComboBoxValueList& values, bool storeValueNotIndex)
{
	wxChoice* choice = new wxChoice(_parent, wxID_ANY);

	// Add all the string values to the combo box
	for (const std::string& value : values)
	{
		choice->Append(value);
	}

	// Connect the registry key to this combo
	registry::bindWidgetToBufferedKey(choice, _registryKey, _buffer, _resetSignal, storeValueNotIndex);

	return choice;
}

wxWindow* PreferenceItem::createPathEntry(bool browseDirectories)
{
	wxutil::PathEntry* entry = new wxutil::PathEntry(_parent, browseDirectories);

	// Connect the registry key to the newly created input field
	registry::bindWidgetToBufferedKey(entry->getEntryWidget(), _registryKey, _buffer, _resetSignal);

	int minChars = static_cast<int>(std::max(GlobalRegistry().get(_registryKey).size(), std::size_t(30)));

	entry->getEntryWidget()->SetMinClientSize(
		wxSize(entry->getEntryWidget()->GetCharWidth() * minChars, -1));

	// Initialize entry
	entry->setValue(registry::getValue<std::string>(_registryKey));

	return entry;
}

wxWindow* PreferenceItem::createSpinner(double lower, double upper, int fractionInt)
{
	double fraction = static_cast<double>(fractionInt);
	double step = 1.0 / fraction;
	unsigned int digits = 0;

	for (; fraction > 1; fraction /= 10)
	{
		++digits;
	}

	if (digits == 0)
	{
		wxSpinCtrl* spinner = new wxSpinCtrl(_parent, wxID_ANY);

		spinner->SetRange(static_cast<int>(lower), static_cast<int>(upper));
		spinner->SetMinClientSize(wxSize(64, -1));

		// Connect the registry key to the newly created spinbutton
		registry::bindWidgetToBufferedKey(spinner, _registryKey, _buffer, _resetSignal);

		return spinner;
	}
	else
	{
		wxSpinCtrlDouble* spinner = new wxSpinCtrlDouble(_parent, wxID_ANY);

		spinner->SetRange(lower, upper);
		spinner->SetIncrement(step);
		spinner->SetMinClientSize(wxSize(64, -1));

		// Connect the registry key to the newly created spinbutton
		registry::bindWidgetToBufferedKey(spinner, _registryKey, _buffer, _resetSignal);

		return spinner;
	}
}

wxWindow* PreferenceItem::createSlider(double lower, double upper, double stepIncrement, double pageIncrement)
{
	// Since sliders are int only, we need to factor the values to support floats
	int factor = static_cast<int>(1 / stepIncrement);

	wxPanel* panel = new wxPanel(_parent, wxID_ANY);

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	panel->SetSizer(hbox);

	// Get the current value from the registry
	double value = registry::getValue<float>(_registryKey) * factor;

	wxSlider* slider = new wxSlider(panel, wxID_ANY, value * factor, lower * factor, upper * factor);
	slider->SetPageSize(pageIncrement * factor);

	// Add a text widget displaying the value
	wxStaticText* valueLabel = new wxStaticText(panel, wxID_ANY, "");
	slider->Bind(wxEVT_SCROLL_CHANGED, [=](wxScrollEvent& ev)
	{
		valueLabel->SetLabelText(string::to_string(slider->GetValue()));
		ev.Skip();
	});

	slider->Bind(wxEVT_SCROLL_THUMBTRACK, [=](wxScrollEvent& ev)
	{
		valueLabel->SetLabelText(string::to_string(slider->GetValue()));
		ev.Skip();
	});

	valueLabel->SetLabelText(string::to_string(value));

	hbox->Add(valueLabel, 0, wxALIGN_CENTER_VERTICAL);
	hbox->Add(slider, 1, wxEXPAND | wxLEFT, 6);

	// Connect the registry key to this slider
	registry::bindWidgetToBufferedKey(slider, _registryKey, _buffer, _resetSignal, factor);

	// Update the value text now, the wxSlider::SetValue() doesn't trigger the changed signal
	valueLabel->SetLabelText(string::to_string(slider->GetValue()));

	return panel;
}

} // namespace
