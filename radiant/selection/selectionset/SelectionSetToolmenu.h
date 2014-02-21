#pragma once

#include "iselectionset.h"
#include <boost/shared_ptr.hpp>
#include <wx/combobox.h>

class wxToolBar;
class wxCommandEvent;

namespace selection
{

class SelectionSetToolmenu :
	public ISelectionSetManager::Observer,
	public wxComboBox
{
public:
	SelectionSetToolmenu(wxToolBar* parent);

	virtual ~SelectionSetToolmenu();

	// Observer implementation
	void onSelectionSetsChanged();

private:
	// Updates the available list items and widget sensitivity
	void update();

	void onSelectionChanged(wxCommandEvent& ev);
	void onEntryActivated(wxCommandEvent& ev);
};

} // namespace selection
