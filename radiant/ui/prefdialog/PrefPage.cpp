#include "PrefPage.h"

#include "i18n.h"

#include <wx/sizer.h>
#include <wx/stattext.h>

#include "settings/PreferenceItems.h"
#include "PreferenceItem.h"

namespace ui 
{

PrefPage::PrefPage(wxWindow* parent, const settings::PreferencePage& settingsPage) :
	wxScrolledWindow(parent, wxID_ANY),
	_settingsPage(settingsPage)
{
	// Create the overall panel
	SetScrollRate(0, 3);
	SetSizer(new wxBoxSizer(wxVERTICAL));

	// 12 pixel border
	wxBoxSizer* overallVBox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(overallVBox, 1, wxEXPAND | wxALL, 12);

	// Create the label, unless the page is empty
	if (!settingsPage.isEmpty())
	{
		wxStaticText* titleLabel = new wxStaticText(this, wxID_ANY, _settingsPage.getTitle());
		titleLabel->SetFont(titleLabel->GetFont().Bold());
		overallVBox->Add(titleLabel, 0, wxBOTTOM, 12);
	}

	_table = new wxFlexGridSizer(1, 2, 6, 12);
	overallVBox->Add(_table, 1, wxEXPAND | wxLEFT, 6); // another 12 pixels to the left

	settingsPage.foreachItem([&](const settings::PreferenceItemBasePtr& item)
	{
		createItemWidgets(item);
	});
}

void PrefPage::saveChanges()
{
	_registryBuffer.commitChanges();
}

void PrefPage::resetValues()
{
	_registryBuffer.clear();

	_resetValuesSignal();
}

void PrefPage::appendNamedWidget(const std::string& name, wxWindow* widget, bool useFullWidth)
{
	if (_table->GetItemCount() > 0)
	{
		// Add another row
		_table->SetRows(_table->GetRows() + 1);
	}

	_table->Add(new wxStaticText(this, wxID_ANY, name), 0, wxALIGN_CENTRE_VERTICAL);
	_table->Add(widget, useFullWidth ? 1 : 0, wxEXPAND);
}

void PrefPage::createItemWidgets(const settings::PreferenceItemBasePtr& item)
{
	// Construct a generic item and pass the common values
	PreferenceItem widget(this, item->getRegistryKey(), _registryBuffer, _resetValuesSignal);
	
	// Switch on the item type
	if (std::dynamic_pointer_cast<settings::PreferenceLabel>(item))
	{
		wxWindow* label = widget.createLabel(item->getLabel());

		appendNamedWidget("", label);
	}
	else if (std::dynamic_pointer_cast<settings::PreferenceEntry>(item))
	{
		appendNamedWidget(item->getLabel(), widget.createEntry());
	}
	else if (std::dynamic_pointer_cast<settings::PreferenceCheckbox>(item))
	{
		wxWindow* checkbox = widget.createCheckbox(item->getLabel());

		appendNamedWidget("", checkbox);
	}
	else if (std::dynamic_pointer_cast<settings::PreferenceCombobox>(item))
	{
		std::shared_ptr<settings::PreferenceCombobox> info = std::static_pointer_cast<settings::PreferenceCombobox>(item);

		wxWindow* combobox = widget.createCombobox(info->getValues(), info->storeValueNotIndex());

		appendNamedWidget(item->getLabel(), combobox, false);
	}
	else if (std::dynamic_pointer_cast<settings::PreferencePathEntry>(item))
	{
		std::shared_ptr<settings::PreferencePathEntry> info = std::static_pointer_cast<settings::PreferencePathEntry>(item);

		wxWindow* pathEntry = widget.createPathEntry(info->browseDirectories());

		appendNamedWidget(item->getLabel(), pathEntry);
	}
	else if (std::dynamic_pointer_cast<settings::PreferenceSpinner>(item))
	{
		std::shared_ptr<settings::PreferenceSpinner> info = std::static_pointer_cast<settings::PreferenceSpinner>(item);

		wxWindow* spinner = widget.createSpinner(info->getLower(), info->getUpper(), info->getFraction());

		appendNamedWidget(item->getLabel(), spinner);
	}
	else if (std::dynamic_pointer_cast<settings::PreferenceSlider>(item))
	{
		std::shared_ptr<settings::PreferenceSlider> info = std::static_pointer_cast<settings::PreferenceSlider>(item);

		wxWindow* slider = widget.createSlider(info->getLower(), info->getUpper(), 
			info->getStepIncrement(), info->getPageIncrement());

		appendNamedWidget(item->getLabel(), slider);
	}
}

} // namespace ui
