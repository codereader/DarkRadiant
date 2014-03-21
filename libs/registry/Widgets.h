#pragma once

#include <wx/spinctrl.h>
#include <wx/textctrl.h>

namespace registry
{

/**
 * Various bind() overloads to let widgets export their values to the registry
 * as soon as they fire their changes signal. The value
 * will be initialised with the one currently present in the registry.
 *
 * Note: due to the use of lambdas it's not  possible to  disconnect 
 * the widget's after calling bind(). The widget will keep writing 
 * its value to the registry, unless it's destroyed.
 */
void bindWidget(wxSpinCtrlDouble* spinCtrl, const std::string& key)
{
	// Set initial value then connect to changed signal
	if (GlobalRegistry().keyExists(key))
	{
		spinCtrl->SetValue(registry::getValue<double>(key));
	}

	spinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, [=] (wxSpinDoubleEvent&) { registry::setValue(key, spinCtrl->GetValue()); });
}

void bindWidget(wxTextCtrl* text, const std::string& key)
{
	// Set initial value then connect to changed signal
	if (GlobalRegistry().keyExists(key))
	{
		text->SetValue(registry::getValue<std::string>(key));
	}

	text->Bind(wxEVT_TEXT, [=] (wxCommandEvent&) { registry::setValue(key, text->GetValue().ToStdString()); });
}

} // namespace
