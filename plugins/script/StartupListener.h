#ifndef _STARTUP_LISTENER_H_
#define _STARTUP_LISTENER_H_

#include "iradiant.h"

#include "ScriptingSystem.h"

namespace script {

class StartupListener :
	public RadiantEventListener
{
	ScriptingSystem& _owner;

public:
	StartupListener(ScriptingSystem& owner) :
		_owner(owner)
	{}

	void onRadiantStartup() {
		// Pass the call to the owner
		_owner.initialise();
	}
};
typedef boost::shared_ptr<StartupListener> StartupListenerPtr;

} // namespace script

#endif /* _STARTUP_LISTENER_H_ */
