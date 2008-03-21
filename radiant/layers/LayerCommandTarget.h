#ifndef LAYER_COMMAND_TARGET_H_
#define LAYER_COMMAND_TARGET_H_

#include "generic/callback.h"
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
	void addSelectionToLayer();

	// The shortcut typedef for use with the EventManager
	typedef MemberCaller<LayerCommandTarget, 
		&LayerCommandTarget::addSelectionToLayer> AddSelectionCaller;
};
typedef boost::shared_ptr<LayerCommandTarget> LayerCommandTargetPtr;

} // namespace scene

#endif /* LAYER_COMMAND_TARGET_H_ */
