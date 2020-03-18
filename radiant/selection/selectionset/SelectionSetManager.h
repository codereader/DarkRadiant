#pragma once

#include "iselectionset.h"
#include "iradiant.h"
#include "imap.h"
#include "icommandsystem.h"

#include <map>
#include "SelectionSet.h"

#include <wx/event.h>

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

public:
    SelectionSetManager();

	// ISelectionSetManager implementation
    sigc::signal<void> signal_selectionSetsChanged() const override;
	void foreachSelectionSet(Visitor& visitor) override;
	void foreachSelectionSet(const VisitorFunc& functor) override;
	ISelectionSetPtr createSelectionSet(const std::string& name) override;
	void deleteSelectionSet(const std::string& name) override;
	void deleteAllSelectionSets() override;
	ISelectionSetPtr findSelectionSet(const std::string& name) override;

	// Command target
	void deleteAllSelectionSetsCmd(const cmd::ArgumentList& args);
};

} // namespace

