#ifndef _STARTUP_MAP_LOADER_H_
#define _STARTUP_MAP_LOADER_H_

#include "iradiant.h"
#include <boost/shared_ptr.hpp>

namespace map {

class StartupMapLoader :
	public RadiantEventListener
{
public:
	// This gets called as soon as the mainframe starts up
	void onRadiantStartup();

	// Called when the mainframe shuts down
	void onRadiantShutdown();
};
typedef boost::shared_ptr<StartupMapLoader> StartupMapLoaderPtr;

} // namespace map

#endif /* _STARTUP_MAP_LOADER_H_ */
