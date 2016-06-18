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

#include "PreferenceItems.h"

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
	_pageWidget(nullptr),
	_titleLabel(nullptr)
{
	if (parentPage && !parentPage->getPath().empty())
	{
		_path = parentPage->getPath() + "/" + _name;
	}
	else
	{
		_path = _name;
	}
#if 0
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
#endif
}

void PrefPage::setTitle(const std::string& title)
{
	if (_titleLabel != NULL)
	{
		_titleLabel->SetLabelText(title);
	}
}

const std::string& PrefPage::getPath() const
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

wxWindow* PrefPage::createWidget(wxTreebook* parent)
{
	if (_pageWidget != nullptr)
	{
		destroyWidgets();
	}

	// Create the overall panel
	_pageWidget = new wxScrolledWindow(parent, wxID_ANY);
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

	for (const PreferenceItemBasePtr& item : _items)
	{
		wxWindow* itemWidget = item->createWidget(_pageWidget);

		// Connect the widget to the registry now that it's been created
		item->connectWidgetToKey(_registryBuffer, _resetValuesSignal);

		appendNamedWidget(item->getName(), itemWidget, item->useFullWidth());
	}

	return _pageWidget;
}

wxWindow* PrefPage::getWidget()
{
	return _pageWidget;
}

void PrefPage::destroyWidgets()
{
	// Clear all existing connections, the widgets are most likely destroyed
	_resetValuesSignal.clear();

	if (_pageWidget)
	{
		_pageWidget->Destroy();
	}

	_pageWidget = nullptr;
	_titleLabel = nullptr;
	_table = nullptr;
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
	PreferenceItemBasePtr item = std::make_shared<PreferenceCheckbox>(name, flag);

	item->setRegistryKey(registryKey);

	_items.push_back(item);
}

void PrefPage::appendSlider(const std::string& name, const std::string& registryKey, bool drawValue,
                            double value, double lower, double upper, double step_increment, double page_increment, double page_size)
{
	PreferenceItemBasePtr item = std::make_shared<PreferenceSlider>(name, value, lower, upper, step_increment, page_increment);

	item->setRegistryKey(registryKey);

	_items.push_back(item);
}

void PrefPage::appendCombo(const std::string& name,
                           const std::string& registryKey,
                           const ComboBoxValueList& valueList,
                           bool storeValueNotIndex)
{
	PreferenceItemBasePtr item = std::make_shared<PreferenceCombobox>(name, valueList, storeValueNotIndex);

	item->setRegistryKey(registryKey);

	_items.push_back(item);
}

void PrefPage::appendEntry(const std::string& name, const std::string& registryKey)
{
	PreferenceItemBasePtr item = std::make_shared<PreferenceEntry>(name);
	
	item->setRegistryKey(registryKey);

	_items.push_back(item);
}

void PrefPage::appendLabel(const std::string& caption)
{
	_items.push_back(std::make_shared<PreferenceLabel>(caption));
}

void PrefPage::appendPathEntry(const std::string& name, const std::string& registryKey, bool browseDirectories)
{
	PreferenceItemBasePtr item = std::make_shared<PreferencePathEntry>(name, browseDirectories);

	item->setRegistryKey(registryKey);

	_items.push_back(item);
}

void PrefPage::appendSpinner(const std::string& name, const std::string& registryKey,
                                   double lower, double upper, int fraction)
{
	PreferenceItemBasePtr item = std::make_shared<PreferenceSpinner>(name, lower, upper, fraction);

	item->setRegistryKey(registryKey);

	_items.push_back(item);
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
