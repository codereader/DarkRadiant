#pragma once

#include "iselectionset.h"
#include "iradiant.h"
#include "imap.h"
#include "icommandsystem.h"

#include <map>
#include "SelectionSet.h"

#include <wx/event.h>
#include <wx/toolbar.h>

namespace selection
{

class SelectionSetToolmenu;

class SelectionSetManager :
	public ISelectionSetManager,
	public std::enable_shared_from_this<SelectionSetManager>,
	public wxEvtHandler
{
    // Signal emitted when contents changes
    sigc::signal<void> _sigSelectionSetsChanged;

	typedef std::map<std::string, SelectionSetPtr> SelectionSets;
	SelectionSets _selectionSets;

	SelectionSetToolmenu* _toolMenu;
	wxToolBarToolBase* _clearAllButton;

public:
    SelectionSetManager();

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();

	void onRadiantStartup();

	// ISelectionSetManager implementation
    sigc::signal<void> signal_selectionSetsChanged() const;
	void foreachSelectionSet(Visitor& visitor);
	void foreachSelectionSet(const VisitorFunc& functor);
	ISelectionSetPtr createSelectionSet(const std::string& name);
	void deleteSelectionSet(const std::string& name);
	void deleteAllSelectionSets();
	ISelectionSetPtr findSelectionSet(const std::string& name);

	// Command target
	void deleteAllSelectionSetsCmd(const cmd::ArgumentList& args);

private:
	void onMapEvent(IMap::MapEvent ev);
	void onDeleteAllSetsClicked(wxCommandEvent& ev);
};

} // namespace

