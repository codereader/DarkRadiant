#ifndef TEXTOOL_SELECTABLE_H_
#define TEXTOOL_SELECTABLE_H_

#include "math/Vector2.h"
#include "math/aabb.h"
#include "Rectangle.h"

namespace textool {

class Selectable 
{
protected:
	bool _selected;

public:
	Selectable() :
		_selected(false)
	{}

	/** greebo: Tests if this can be selected within the given 
	 * 			rectangle (s/t coordinates).
	 * 
	 * @returns: TRUE if the selectable responds to 
	 * 			 the given rectangle, FALSE otherwise.
	 */
	virtual bool testSelect(const Rectangle& rectangle) {
		// Default implementation: returns FALSE
		return false;
	}
	
	/** greebo: Sets the selection status to <selected>
	 */
	virtual void setSelected(bool selected) {
		_selected = selected;
	}
		
	/** greebo: Returns TRUE if this object is selected
	 */
	virtual bool isSelected() const {
		return _selected;
	}
	
	/** greebo: Toggles the current selection status of this object
	 */
	virtual void toggle() {
		_selected = !_selected;
	}
	
	/** greebo: Retrieves the dimensions of this object in texture space.
	 * 			This returns the AABB with the z-component set to 0.
	 */
	virtual AABB getExtents() = 0;
 
	/** greebo: Retrieves the extents of the selected items only.
	 * 			Useful to retrieve an centroid of all the selected items.
	 */ 	
 	virtual AABB getSelectedExtents() = 0;
 
}; // class Selectable

} // namespace textool

#endif /*TEXTOOL_SELECTABLE_H_*/
