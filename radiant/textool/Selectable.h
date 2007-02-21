#ifndef TEXTOOL_SELECTABLE_H_
#define TEXTOOL_SELECTABLE_H_

namespace selection {
	namespace textool {

class Selectable 
{
	
protected:
	bool _selected;
	 
public:
	Selectable() :
		_selected(false)
	{}

	/** greebo: Tests if this can be selected at the given s/t coordinates.
	 * 
	 * @returns: TRUE if the selectable corresponds to 
	 * 			 the given coords, FALSE otherwise.
	 */
	virtual bool testSelect(const float s, const float& t) = 0;
	
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
