#ifndef _SELECTION_SET_MANAGER_H_
#define _SELECTION_SET_MANAGER_H_

#include "imodule.h"
#include "iradiant.h"

#include <boost/enable_shared_from_this.hpp>

namespace selection
{

class SelectionSetManager :
	public RegisterableModule,
	public RadiantEventListener,
	public boost::enable_shared_from_this<SelectionSetManager>
{
public:
	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();

	void onRadiantStartup();
};

} // namespace

#endif /* _SELECTION_SET_MANAGER_H_ */
