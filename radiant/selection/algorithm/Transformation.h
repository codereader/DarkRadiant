#ifndef SELECTION_TRANSFORMATION_H_
#define SELECTION_TRANSFORMATION_H_

#include "math/Vector3.h"

namespace selection {
	namespace algorithm {
		
		/** greebo: Rotates the current selection about the 
		 * 			specified rotation angles.
		 * 
		 * @eulerXYZ: A three-component vector containing the three
		 * 			  angles in degrees (vector[0] refers to x-axis rotation).
		 * 
		 * Note: this is an undoable command.
		 */
		void rotateSelected(const Vector3& eulerXYZ);
		
		/** greebo: Scales the current selection with the given vector.
		 * 			this emits an error if one of the vector's components
		 * 			are zero.
		 * 
		 * @scaleXYZ: A three-component vector (can be non-uniform) containing
		 * 			  the three scale factors.
		 * 
		 * Note: this is an undoable command.
		 */
		void scaleSelected(const Vector3& scaleXYZ);
		
	} // namespace algorithm
} // namespace selection

#endif /*SELECTION_TRANSFORMATION_H_*/
