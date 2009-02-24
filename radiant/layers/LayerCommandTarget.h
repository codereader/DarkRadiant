#ifndef LAYER_COMMAND_TARGET_H_
#define LAYER_COMMAND_TARGET_H_

#include "icommandsystem.h"
#include <boost/shared_ptr.hpp>

namespace scene {

/** 
 * greebo: This objects is associated with a given Layer ID
 *         and provides methods compatible with the event system.
 *
 * On instantiation, this object self-registers in the EventManager
 */
class LayerCommandTarget {
	// The layer ID this command target is associated with
	int _layerID;
public:
	// Constructor, registers this command target
	LayerCommandTarget(int layerID);

	// Command target, this adds the current selection to the associated layer
	void addSelectionToLayer(const cmd::ArgumentList& args);
	void moveSelectionToLayer(const cmd::ArgumentList& args);

	// Command targets, hides/shows the associated layer
	void showLayer(const cmd::ArgumentList& args);
	void hideLayer(const cmd::ArgumentList& args);
};
typedef boost::shared_ptr<LayerCommandTarget> LayerCommandTargetPtr;

} // namespace scene

#endif /* LAYER_COMMAND_TARGET_H_ */
