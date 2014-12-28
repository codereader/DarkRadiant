#pragma once

#include "iselectionset.h"
#include <memory>
#include <wx/combobox.h>

class wxToolBar;
class wxCommandEvent;

namespace selection
{

class SelectionSetToolmenu: public wxComboBox
{
public:
	SelectionSetToolmenu(wxToolBar* parent);

private:
	// Updates the available list items and widget sensitivity
	void update();

	void onSelectionChanged(wxCommandEvent& ev);
	void onEntryActivated(wxCommandEvent& ev);
};

} // namespace selection
