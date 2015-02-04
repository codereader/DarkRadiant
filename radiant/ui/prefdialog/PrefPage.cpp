#include "PrefPage.h"

#include "i18n.h"
#include "itextstream.h"
#include "registry/buffer.h"
#include "registry/registry.h"
#include "registry/Widgets.h"

#include "wxutil/PathEntry.h"

#include <iostream>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/format.hpp>
#include "modulesystem/ApplicationContextImpl.h"

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/treebook.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/scrolwin.h>

namespace ui 
{

namespace 
{
	typedef std::vector<std::string> StringVector;
}

PrefPage::PrefPage(const std::string& name,
                   wxTreebook* notebook,
				   const PrefPagePtr& parentPage) : 
	_name(name), 
	_notebook(notebook),
	_pageWidget(NULL),
	_titleLabel(NULL)
{
	if (parentPage && !parentPage->getPath().empty())
	{
		_path = parentPage->getPath() + "/" + _name;
	}
	else
	{
		_path = _name;
	}

	if (!_name.empty())
	{
		// Create the overall panel
        _pageWidget = new wxScrolledWindow(_notebook, wxID_ANY);
        _pageWidget->SetScrollRate(0, 3);

		_pageWidget->SetSizer(new wxBoxSizer(wxVERTICAL));

		// 12 pixel border
		wxBoxSizer* overallVBox = new wxBoxSizer(wxVERTICAL);
		_pageWidget->GetSizer()->Add(overallVBox, 1, wxEXPAND | wxALL, 12);

		// Create the label
		_titleLabel = new wxStaticText(_pageWidget, wxID_ANY, 
			(boost::format("%s Settings") % _name).str()
		);
		_titleLabel->SetFont(_titleLabel->GetFont().Bold());
		overallVBox->Add(_titleLabel, 0, wxBOTTOM, 12);

		_table = new wxFlexGridSizer(1, 2, 6, 12);
        overallVBox->Add(_table, 1, wxEXPAND | wxLEFT, 6); // another 12 pixels to the left

		if (parentPage && !parentPage->getName().empty())
		{
			// Find the index of the parent page to perform the insert
			int pos = _notebook->FindPage(parentPage->getWidget());
			_notebook->InsertSubPage(pos, _pageWidget, name);
		}
		else if (!_name.empty())
		{
			// Append the panel as new page to the notebook
			_notebook->AddPage(_pageWidget, name);
		}
	}
}

void PrefPage::setTitle(const std::string& title)
{
	if (_titleLabel != NULL)
	{
		_titleLabel->SetLabelText(title);
	}
}

std::string PrefPage::getPath() const
{
	return _path;
}

std::string PrefPage::getName() const
{
	return _name;
}

void PrefPage::saveChanges()
{
	_registryBuffer.commitChanges();
}

void PrefPage::discardChanges()
{
	_registryBuffer.clear();

	_resetValuesSignal();
}

wxWindow* PrefPage::getWidget()
{
	return _pageWidget;
}

void PrefPage::foreachPage(const std::function<void(PrefPage&)>& functor)
{
	std::for_each(_children.begin(), _children.end(), [&] (const PrefPagePtr& page)
	{
		// Visit this instance
		functor(*page);

		// Pass the visitor recursively
		page->foreachPage(functor);
	});
}

void PrefPage::appendCheckBox(const std::string& name,
                              const std::string& flag,
                              const std::string& registryKey)
{
	// Create a new checkbox with the given caption and display it
	wxCheckBox* check = new wxCheckBox(_pageWidget, wxID_ANY, flag);

	// Connect the registry key to this toggle button
    registry::bindWidgetToBufferedKey(check, registryKey, _registryBuffer, _resetValuesSignal);

	appendNamedWidget(name, check);
}

void PrefPage::appendSlider(const std::string& name, const std::string& registryKey, bool drawValue,
                            double value, double lower, double upper, double step_increment, double page_increment, double page_size)
{
	// Since sliders are int only, we need to factor the values to support floats
	int factor = static_cast<int>(1 / step_increment);

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

	wxSlider* slider = new wxSlider(_pageWidget, wxID_ANY, value * factor, lower * factor, upper * factor);
	slider->SetPageSize(page_increment * factor);

	// Add a text widget displaying the value
	wxStaticText* valueText = new wxStaticText(_pageWidget, wxID_ANY, "");
	slider->Bind(wxEVT_SCROLL_CHANGED, [=] (wxScrollEvent& ev)
	{ 
		valueText->SetLabelText(string::to_string(slider->GetValue())); 
		ev.Skip();
	});
	slider->Bind(wxEVT_SCROLL_THUMBTRACK, [=] (wxScrollEvent& ev)
	{ 
		valueText->SetLabelText(string::to_string(slider->GetValue())); 
		ev.Skip();
	});
	valueText->SetLabelText(string::to_string(value));

	hbox->Add(valueText, 0, wxALIGN_CENTER_VERTICAL);
	hbox->Add(slider, 1, wxEXPAND | wxLEFT, 6);

	// Connect the registry key to this adjustment
    registry::bindWidgetToBufferedKey(slider, registryKey, _registryBuffer, _resetValuesSignal, factor);

	appendNamedSizer(name, hbox);
}

void PrefPage::appendCombo(const std::string& name,
                           const std::string& registryKey,
                           const ComboBoxValueList& valueList,
                           bool storeValueNotIndex)
{
	wxChoice* choice = new wxChoice(_pageWidget, wxID_ANY);

    // Add all the string values to the combo box
    for (ComboBoxValueList::const_iterator i = valueList.begin();
         i != valueList.end();
         ++i)
    {
		choice->Append(*i);
    }

	registry::bindWidgetToBufferedKey(choice, registryKey, _registryBuffer, _resetValuesSignal, storeValueNotIndex);

	// Add the widget to the dialog row
	appendNamedWidget(name, choice, false);
}

void PrefPage::appendEntry(const std::string& name, const std::string& registryKey)
{
	wxTextCtrl* entry = new wxTextCtrl(_pageWidget, wxID_ANY);

	int minChars = static_cast<int>(std::max(GlobalRegistry().get(registryKey).size(), std::size_t(30)));
	entry->SetMinClientSize(wxSize(entry->GetCharWidth() * minChars, -1));

	// Connect the registry key to the newly created input field
    registry::bindWidgetToBufferedKey(entry, registryKey, _registryBuffer, _resetValuesSignal);

	appendNamedWidget(name, entry);
}

void PrefPage::appendLabel(const std::string& caption)
{
	wxStaticText* label = new wxStaticText(_pageWidget, wxID_ANY, "");
	label->SetLabelMarkup(caption);

	appendNamedWidget("", label);
}

void PrefPage::appendPathEntry(const std::string& name, const std::string& registryKey, bool browseDirectories)
{
	wxutil::PathEntry* entry = new wxutil::PathEntry(_pageWidget, browseDirectories);

	// Connect the registry key to the newly created input field
    registry::bindWidgetToBufferedKey(entry->getEntryWidget(),
                                registryKey, _registryBuffer, _resetValuesSignal);

	int minChars = static_cast<int>(std::max(GlobalRegistry().get(registryKey).size(), std::size_t(30)));

	entry->getEntryWidget()->SetMinClientSize(
		wxSize(entry->getEntryWidget()->GetCharWidth() * minChars, -1));

	// Initialize entry
	entry->setValue(registry::getValue<std::string>(registryKey));

	appendNamedWidget(name, entry);
}

void PrefPage::appendSpinner(const std::string& name, const std::string& registryKey,
                                   double lower, double upper, int fraction)
{
	// Load the initial value (maybe unnecessary, as the value is loaded upon dialog show)
	float value = registry::getValue<float>(registryKey);

	double step = 1.0 / static_cast<double>(fraction);
	unsigned int digits = 0;

	for (;fraction > 1; fraction /= 10)
	{
		++digits;
	}

	if (digits == 0)
	{
		wxSpinCtrl* spinner = new wxSpinCtrl(_pageWidget, wxID_ANY);

		spinner->SetRange(static_cast<int>(lower), static_cast<int>(upper));
		spinner->SetValue(static_cast<int>(value));

		spinner->SetMinClientSize(wxSize(64, -1));

		// Connect the registry key to the newly created input field
		registry::bindWidgetToBufferedKey(spinner, registryKey, _registryBuffer, _resetValuesSignal);

		appendNamedWidget(name, spinner);
	}
	else
	{
		wxSpinCtrlDouble* spinner = new wxSpinCtrlDouble(_pageWidget, wxID_ANY);

		spinner->SetRange(lower, upper);
		spinner->SetValue(value);
		spinner->SetIncrement(step);

		spinner->SetMinClientSize(wxSize(64, -1));

		// Connect the registry key to the newly created input field
		registry::bindWidgetToBufferedKey(spinner, registryKey, _registryBuffer, _resetValuesSignal);

		appendNamedWidget(name, spinner);
	}
}

PrefPagePtr PrefPage::createOrFindPage(const std::string& path)
{
	// Split the path into parts
	StringVector parts;
	boost::algorithm::split(parts, path, boost::algorithm::is_any_of("/"));

	if (parts.empty())
	{
        rConsole() << "Warning: Could not resolve preference path: " << path << std::endl;
		return PrefPagePtr();
	}

	PrefPagePtr child;

	// Try to lookup the page in the child list
	for (std::size_t i = 0; i < _children.size(); ++i)
	{
		if (_children[i]->getName() == parts[0])
		{
			child = _children[i];
			break;
		}
	}

	if (child == NULL)
	{
		// No child found, create a new page and add it to the list
		child = PrefPagePtr(new PrefPage(parts[0], _notebook, shared_from_this()));
		_children.push_back(child);
	}

	// We now have a child with this name, do we have a leaf?
	if (parts.size() > 1) {
		// We have still more parts, split off the first part
		std::string subPath("");
		for (std::size_t i = 1; i < parts.size(); ++i)
		{
			subPath += (subPath.empty()) ? "" : "/";
			subPath += parts[i];
		}
		// Pass the call to the child
		return child->createOrFindPage(subPath);
	}
	else {
		// We have found a leaf, return the child page
		return child;
	}
}

void PrefPage::appendNamedWidget(const std::string& name, wxWindow* widget, bool useFullWidth)
{
	if (_table->GetItemCount() > 0)
	{
		// Add another row
		_table->SetRows(_table->GetRows() + 1);
	}

	_table->Add(new wxStaticText(_pageWidget, wxID_ANY, name), 0, wxALIGN_CENTRE_VERTICAL);
	_table->Add(widget, useFullWidth ? 1 : 0, wxEXPAND);
}

void PrefPage::appendNamedSizer(const std::string& name, wxSizer* sizer, bool useFullWidth)
{
	if (_table->GetItemCount() > 0)
	{
		// Add another row
		_table->SetRows(_table->GetRows() + 1);
	}

	_table->Add(new wxStaticText(_pageWidget, wxID_ANY, name), 0, wxALIGN_CENTRE_VERTICAL);
	_table->Add(sizer, useFullWidth ? 1 : 0, wxEXPAND);
}

} // namespace ui
