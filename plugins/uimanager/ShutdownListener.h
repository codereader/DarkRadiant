#ifndef _SHUTDOWN_LISTENER_H_
#define _SHUTDOWN_LISTENER_H_

#include "iradiant.h"
#include "UIManager.h"

namespace ui {

/**
 * A small class listening for the Radiant shutdown event.
 */
class UIManagerShutdownListener :
	public RadiantEventListener
{
	UIManager& _uiManager;
public:
	UIManagerShutdownListener(UIManager& uiManager) :
		_uiManager(uiManager)
	{}

	void onRadiantShutdown() {
		// Clear the menu manager on shutdown
		_uiManager.clear();
	}
};
typedef boost::shared_ptr<UIManagerShutdownListener> UIManagerShutdownListenerPtr;

} // namespace ui

#endif /* _SHUTDOWN_LISTENER_H_ */
