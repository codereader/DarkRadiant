#include "SelectionSetManager.h"

#include "itextstream.h"
#include "i18n.h"
#include "iselection.h"
#include "idialogmanager.h"
#include "imapinfofile.h"
#include "iuimanager.h"
#include "ieventmanager.h"
#include "imainframe.h"
#include "modulesystem/StaticModule.h"
#include "SelectionSetToolmenu.h"
#include "SelectionSetInfoFileModule.h"

#include <wx/toolbar.h>
#include <wx/frame.h>
#include <wx/artprov.h>
#include <wx/stattext.h>

#include <functional>

namespace selection
{

SelectionSetManager::SelectionSetManager()
{}

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

#if 0 // TODO: move to separate class
        if (_clearAllButton)
        {
            _clearAllButton->GetToolBar()->EnableTool(_clearAllButton->GetId(), !_selectionSets.empty());
        }
#endif
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

#if 0 // TODO: move to separate class
        if (_clearAllButton)
        {
            _clearAllButton->GetToolBar()->EnableTool(_clearAllButton->GetId(), !_selectionSets.empty());
        }
#endif
    }
}

void SelectionSetManager::deleteAllSelectionSets()
{
	_selectionSets.clear();
	_sigSelectionSetsChanged();

#if 0 // TODO: move to separate class
    if (_clearAllButton)
    {
        _clearAllButton->GetToolBar()->EnableTool(_clearAllButton->GetId(), false);
    }
#endif
}

ISelectionSetPtr SelectionSetManager::findSelectionSet(const std::string& name)
{
	auto i = _selectionSets.find(name);

	return i != _selectionSets.end() ? i->second : ISelectionSetPtr();
}

} // namespace
