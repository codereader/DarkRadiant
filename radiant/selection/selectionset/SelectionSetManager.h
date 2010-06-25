#ifndef _SELECTION_SET_MANAGER_H_
#define _SELECTION_SET_MANAGER_H_

#include "iselectionset.h"
#include "iradiant.h"

#include <map>
#include "SelectionSet.h"
#include "SelectionSetToolmenu.h"

#include <boost/enable_shared_from_this.hpp>

namespace selection
{

class SelectionSetManager :
	public ISelectionSetManager,
	public RadiantEventListener,
	public boost::enable_shared_from_this<SelectionSetManager>
{
private:
	SelectionSetToolmenuPtr _toolmenu;

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
	ISelectionSetPtr createSelectionSet(const std::string& name);
	void deleteSelectionSet(const std::string& name);
	ISelectionSetPtr findSelectionSet(const std::string& name);
};

} // namespace

#endif /* _SELECTION_SET_MANAGER_H_ */
