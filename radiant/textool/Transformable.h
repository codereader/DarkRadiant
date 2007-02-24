#ifndef TEXTOOL_TRANSFORMABLE_H_
#define TEXTOOL_TRANSFORMABLE_H_

#include "Selectable.h"

class Matrix4;

namespace selection {
	namespace textool {

class Transformable :
	public Selectable
{
public:
	/** greebo: This transforms the object using the given transformation matrix.
	 * 
	 * @transform: A 4x4 transformation matrix with the z-relevant transformation
	 * 			   components set to identity. 
	 * 
	 * 			   xx, xy, yx, yy, tx, ty are the relevant components.
	 * 
	 * Note: Hm, maybe a Matrix3 would be more appropriate for texture transformations (TODO). 
	 */
	virtual void transform(const Matrix4& matrix) = 0;
	
	/** greebo: Same as above, but applies only to selected items and subitems.
	 *  		This is some sort of "propagation" transformation, that
	 * 			applies to this object (if selected) and all selected subitems.
	 */
	virtual void transformSelected(const Matrix4& matrix) = 0;
	
	/** greebo: Moves the selected items to the specified <targetCoords>
	 * 			This is an absolute operation, not a relative transformation. 			
	 */
	virtual void moveSelectedTo(const Vector2& targetCoords) = 0;
	
	/** greebo: Flips the selected coords about the given axis in texspace. 
	 * 
	 *  @axis:  0 = s-axis flip, 1 = t-axis flip 			
	 */
	virtual void flipSelected(const int& axis) = 0;
	
	/** greebo: Snaps this item to the grid.
	 */
	virtual void snapSelectedToGrid(float grid) = 0;
	
	/** greebo: Tells the items that a transformation is about to begin.
	 * 			This usually triggers an undoSave() which saves the current
	 * 			item state.
	 */
	virtual void beginTransformation() {
		// Empty implementation for the moment being
	}
	
	/** greebo: The counterpart of the above. Finishes the move. 
	 */
	virtual void endTransformation() {
		// Empty implementation for the moment being
	}
};

	} // namespace TexTool
} // namespace selection

#endif /*TEXTOOL_TRANSFORMABLE_H_*/
