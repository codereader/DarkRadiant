#ifndef TEXTOOL_SELECTABLE_H_
#define TEXTOOL_SELECTABLE_H_

#include "math/Vector2.h"

namespace selection {


/** greebo: A structure defining a 2D rectangular shape 
 * 			by specifying the top left and the 
 * 			bottom right corner coordinates.
 */
struct Rectangle {
	Vector2 topLeft;
	Vector2 bottomRight;
};

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
	virtual bool testSelect(const Rectangle& rectangle) = 0;
	
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
 
}; // class Selectable

} // namespace TexTool

} // namespace selection

#endif /*TEXTOOL_SELECTABLE_H_*/
