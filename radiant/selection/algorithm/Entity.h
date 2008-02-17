#ifndef SELECTION_ALGORITHM_ENTITY_H_
#define SELECTION_ALGORITHM_ENTITY_H_

#include <string>

namespace selection {
	namespace algorithm {

	/**
	 * greebo: Changes the classname of the currently selected entities.
	 */
	void setEntityClassname(const std::string& classname);

	} // namespace algorithm
} // namespace selection

#endif /* SELECTION_ALGORITHM_ENTITY_H_ */
