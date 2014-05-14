#pragma once

#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/choice.h>

#include "buffer.h"

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
inline void bindWidget(wxSpinCtrlDouble* spinCtrl, const std::string& key)
{
	// Set initial value then connect to changed signal
	if (GlobalRegistry().keyExists(key))
	{
		spinCtrl->SetValue(registry::getValue<double>(key));
	}

	spinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, [=] (wxSpinDoubleEvent& ev) 
	{
		registry::setValue(key, spinCtrl->GetValue());
		ev.Skip();
	});
}

inline void bindWidget(wxTextCtrl* text, const std::string& key)
{
	// Set initial value then connect to changed signal
	if (GlobalRegistry().keyExists(key))
	{
		text->SetValue(registry::getValue<std::string>(key));
	}

	text->Bind(wxEVT_TEXT, [=] (wxCommandEvent& ev)
	{ 
		registry::setValue(key, text->GetValue().ToStdString());
		ev.Skip();
	});
}

// ------------- Variants supporting registry::Buffer ---------------------

inline void bindWidgetToBufferedKey(wxCheckBox* checkbox, const std::string& key, 
							 Buffer& buffer, sigc::signal<void>& resetSignal)
{
	// Set initial value then connect to changed signal
	checkbox->SetValue(GlobalRegistry().get(key) == "1");

	checkbox->Bind(wxEVT_CHECKBOX, [=, &buffer] (wxCommandEvent& ev)
	{ 
		buffer.set(key, checkbox->GetValue() ? "1" : "0"); 
		ev.Skip();
	});

	resetSignal.connect([=, &buffer] { if (buffer.keyExists(key)) { checkbox->SetValue(buffer.get(key) == "1"); } });
}

inline void bindWidgetToBufferedKey(wxSlider* slider, const std::string& key, 
							 Buffer& buffer, sigc::signal<void>& resetSignal, int factor)
{
	// Set initial value then connect to changed signal
	slider->SetValue(registry::getValue<float>(key) * factor);

	slider->Bind(wxEVT_SCROLL_CHANGED, [=, &buffer] (wxScrollEvent& ev)
	{ 
		buffer.set(key, string::to_string(static_cast<float>(slider->GetValue()) / factor)); 
		ev.Skip();
	});
	slider->Bind(wxEVT_SCROLL_THUMBTRACK, [=, &buffer] (wxScrollEvent& ev)
	{ 
		buffer.set(key, string::to_string(static_cast<float>(slider->GetValue()) / factor)); 
		ev.Skip();
	});

	resetSignal.connect([=, &buffer] { if (buffer.keyExists(key)) { slider->SetValue(registry::getValue<float>(key) * factor); } });
}

inline void bindWidgetToBufferedKey(wxChoice* choice, const std::string& key, 
							 Buffer& buffer, sigc::signal<void>& resetSignal, bool storeValueNotIndex)
{
	// Set initial value then connect to changed signal
	choice->Select(storeValueNotIndex ? 
		choice->FindString(registry::getValue<std::string>(key)):
		registry::getValue<int>(key));

	choice->Bind(wxEVT_CHOICE, [=, &buffer] (wxCommandEvent& ev)
	{ 
		if (storeValueNotIndex)
		{
			buffer.set(key, choice->GetStringSelection().ToStdString()); 
		}
		else
		{
			buffer.set(key, string::to_string(choice->GetSelection()));
		}

		ev.Skip();
	});

	resetSignal.connect([=, &buffer]
	{
		if (buffer.keyExists(key))
		{ 
			choice->Select(storeValueNotIndex ? 
				choice->FindString(registry::getValue<std::string>(key)):
				registry::getValue<int>(key));
		}
	});
}

inline void bindWidgetToBufferedKey(wxTextCtrl* entry, const std::string& key, 
							 Buffer& buffer, sigc::signal<void>& resetSignal)
{
	// Set initial value then connect to changed signal
	if (GlobalRegistry().keyExists(key))
	{
		entry->SetValue(registry::getValue<std::string>(key));
	}

	entry->Bind(wxEVT_TEXT, [=, &buffer] (wxCommandEvent& ev)
	{ 
		buffer.set(key, entry->GetValue().ToStdString());
		ev.Skip();
	});

	resetSignal.connect([=, &buffer]
	{
		if (buffer.keyExists(key))
		{ 
			entry->SetValue(registry::getValue<std::string>(key));
		}
	});
}

} // namespace
