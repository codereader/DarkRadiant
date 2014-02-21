#pragma once

#include "iselectionset.h"
#include <boost/shared_ptr.hpp>
#include <wx/combobox.h>

class wxWindow;

namespace selection
{

class SelectionSetToolmenu :
	public ISelectionSetManager::Observer,
	public wxComboBox
{
private:
	//Gtk::ToolButton* _clearSetsButton;

public:
	SelectionSetToolmenu(wxWindow* parent);

	virtual ~SelectionSetToolmenu();

	// Observer implementation
	void onSelectionSetsChanged();

private:
	// Updates the available list items and widget sensitivity
	void update();

	void onSelectionChanged();
	void onEntryActivated();
	void onDeleteAllSetsClicked();
};

} // namespace selection
