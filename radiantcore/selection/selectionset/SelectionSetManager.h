#pragma once

#include "iselectionset.h"

#include <map>
#include "SelectionSet.h"

#include <sigc++/signal.h>

namespace selection
{

class SelectionSetToolmenu;

class SelectionSetManager :
	public ISelectionSetManager,
	public std::enable_shared_from_this<SelectionSetManager>
{
    // Signal emitted when contents changes
    sigc::signal<void> _sigSelectionSetsChanged;

	typedef std::map<std::string, SelectionSetPtr> SelectionSets;
	SelectionSets _selectionSets;

public:
	// ISelectionSetManager implementation
    sigc::signal<void> signal_selectionSetsChanged() const override;
	void foreachSelectionSet(Visitor& visitor) override;
	void foreachSelectionSet(const VisitorFunc& functor) override;
	ISelectionSetPtr createSelectionSet(const std::string& name) override;
	void deleteSelectionSet(const std::string& name) override;
	void deleteAllSelectionSets() override;
	ISelectionSetPtr findSelectionSet(const std::string& name) override;
};

} // namespace

