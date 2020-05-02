#pragma once

#include <wx/scrolwin.h>

#include "registry/buffer.h"
#include "settings/PreferencePage.h"

class wxFlexGridSizer;

namespace ui
{

/**
 * A preference page as inserted into the PrefDialog's treebook control.
 * Each PrefPage renders the items found in the assigned settings::PreferencePage.
 */
class PrefPage :
	public wxScrolledWindow
{
private:
	// The settings page we're representing
	const IPreferencePage& _settingsPage;

	// We're holding back any registry write operations until the user clicks OK
	registry::Buffer _registryBuffer;

	// A signal chain all registry key-bound widgets are connected with
	// when emitted, the widgets reload the values from the registry.
	sigc::signal<void> _resetValuesSignal;

	// The table this page is adding the widgets to
	wxFlexGridSizer* _table;

public:
	PrefPage(wxWindow* parent, const IPreferencePage& settingsPage);

	// Commit all pending registry write operations.
	void saveChanges();

	// Discard all pending registry write operations.
	void resetValues();

private:
	void appendNamedWidget(const std::string& name, wxWindow* widget, bool useFullWidth = true);

	void createItemWidgets(const IPreferenceItemBase::Ptr& item);
};
typedef std::shared_ptr<PrefPage> PrefPagePtr;

} // namespace ui
