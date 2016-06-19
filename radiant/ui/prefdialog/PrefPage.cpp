#include "PrefPage.h"

#include "i18n.h"

#include <wx/sizer.h>
#include <wx/stattext.h>

#include "PreferenceItems.h"

namespace ui 
{

PrefPage::PrefPage(wxWindow* parent, const settings::PreferencePage& settingsPage) :
	wxScrolledWindow(parent, wxID_ANY),
	_settingsPage(settingsPage),
	_titleLabel(nullptr)
{
	// Create the overall panel
	SetScrollRate(0, 3);
	SetSizer(new wxBoxSizer(wxVERTICAL));

	// 12 pixel border
	wxBoxSizer* overallVBox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(overallVBox, 1, wxEXPAND | wxALL, 12);

	// Create the label
	_titleLabel = new wxStaticText(this, wxID_ANY, (boost::format("%s Settings") % _settingsPage.getName()).str());
	_titleLabel->SetFont(_titleLabel->GetFont().Bold());
	overallVBox->Add(_titleLabel, 0, wxBOTTOM, 12);

	_table = new wxFlexGridSizer(1, 2, 6, 12);
	overallVBox->Add(_table, 1, wxEXPAND | wxLEFT, 6); // another 12 pixels to the left

	/*
	for (const PreferenceItemBasePtr& item : _items)
	{
		wxWindow* itemWidget = item->createWidget(_pageWidget);

		// Connect the widget to the registry now that it's been created
		item->connectWidgetToKey(_registryBuffer, _resetValuesSignal);

		appendNamedWidget(item->getName(), itemWidget, item->useFullWidth());
	}
	*/
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

} // namespace ui
