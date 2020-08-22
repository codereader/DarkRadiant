#pragma once

#include "iselectionset.h"
#include "imap.h"
#include <memory>
#include <sigc++/connection.h>

class wxToolBar;
class wxComboBox;
class wxCommandEvent;
class wxToolBarToolBase;

namespace ui
{

class SelectionSetToolmenu
{
private:
	wxComboBox* _dropdown;
	sigc::connection _setsChangedSignal;
	sigc::connection _mapEventHandler;
	int _dropdownToolId;

	wxToolBarToolBase* _clearAllButton;

	// Constructed and destructed during the Radiant startup/shutdown events
	static std::unique_ptr<SelectionSetToolmenu> _instance;

public:
	SelectionSetToolmenu();
	~SelectionSetToolmenu();

	static void Init();

private:
	// Updates the available list items and widget sensitivity
	void update();

	void onSelectionChanged(wxCommandEvent& ev);
	void onEntryActivated(wxCommandEvent& ev);
	void onDeleteAllSetsClicked(wxCommandEvent& ev);

	void onMapEvent(IMap::MapEvent ev);
	void onRadiantShutdown();

	void connectToMapRoot();
	void disconnectFromMapRoot();
};

} // namespace selection
