#include "SelectionSetManager.h"

#include <functional>

namespace selection
{

sigc::signal<void> SelectionSetManager::signal_selectionSetsChanged() const
{
    return _sigSelectionSetsChanged;
}

void SelectionSetManager::foreachSelectionSet(const VisitorFunc& functor)
{
	for (SelectionSets::const_iterator i = _selectionSets.begin(); i != _selectionSets.end(); )
	{
		functor((i++)->second);
	}
}

void SelectionSetManager::foreachSelectionSet(Visitor& visitor)
{
	foreachSelectionSet([&] (const ISelectionSetPtr& set)
	{
		visitor.visit(set);
	});
}

ISelectionSetPtr SelectionSetManager::createSelectionSet(const std::string& name)
{
	auto i = _selectionSets.find(name);

	if (i == _selectionSets.end())
	{
		// Create new set
		auto result = _selectionSets.emplace(name, std::make_shared<SelectionSet>(name));

		i = result.first;

		_sigSelectionSetsChanged();
	}

	return i->second;
}

void SelectionSetManager::deleteSelectionSet(const std::string& name)
{
	SelectionSets::iterator i = _selectionSets.find(name);

    if (i != _selectionSets.end())
    {
        _selectionSets.erase(i);

        _sigSelectionSetsChanged();
    }
}

void SelectionSetManager::deleteAllSelectionSets()
{
	_selectionSets.clear();

	_sigSelectionSetsChanged();
}

ISelectionSetPtr SelectionSetManager::findSelectionSet(const std::string& name)
{
	auto i = _selectionSets.find(name);

	return i != _selectionSets.end() ? i->second : ISelectionSetPtr();
}

} // namespace
