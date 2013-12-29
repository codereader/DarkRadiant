#ifndef _SELECTION_SET_MANAGER_H_
#define _SELECTION_SET_MANAGER_H_

#include "iselectionset.h"
#include "iradiant.h"
#include "icommandsystem.h"

#include <map>
#include "SelectionSet.h"

#include <boost/enable_shared_from_this.hpp>

namespace selection
{

class SelectionSetManager :
	public ISelectionSetManager,
	public boost::enable_shared_from_this<SelectionSetManager>
{
private:
	typedef std::set<ISelectionSetManager::Observer*> Observers;
	Observers _observers;

	typedef std::map<std::string, SelectionSetPtr> SelectionSets;
	SelectionSets _selectionSets;

public:
	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();

	void onRadiantStartup();

	// ISelectionManager implementation
	void addObserver(Observer& observer);
	void removeObserver(Observer& observer);

	void foreachSelectionSet(Visitor& visitor);
	void foreachSelectionSet(const VisitorFunc& functor);
	ISelectionSetPtr createSelectionSet(const std::string& name);
	void deleteSelectionSet(const std::string& name);
	void deleteAllSelectionSets();
	ISelectionSetPtr findSelectionSet(const std::string& name);

	// Command target
	void deleteAllSelectionSets(const cmd::ArgumentList& args);

private:
	void notifyObservers();
};

} // namespace

#endif /* _SELECTION_SET_MANAGER_H_ */
