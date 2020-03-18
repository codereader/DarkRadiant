#pragma once

#include "iselectionset.h"
#include "imap.h"
#include <memory>
#include <wx/combobox.h>
#include <sigc++/connection.h>

class wxToolBar;
class wxCommandEvent;

namespace selection
{

class SelectionSetToolmenu : 
	public wxComboBox
{
private:
	sigc::connection _setsChangedSignal;

public:
	SelectionSetToolmenu(wxToolBar* parent);

private:
	// Updates the available list items and widget sensitivity
	void update();

	void onSelectionChanged(wxCommandEvent& ev);
	void onEntryActivated(wxCommandEvent& ev);

	void onMapEvent(IMap::MapEvent ev);
	void connectToMapRoot();
	void disconnectFromMapRoot();
};

} // namespace selection
