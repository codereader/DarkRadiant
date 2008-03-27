#ifndef RECTANGLE_H_
#define RECTANGLE_H_

#include "math/Vector2.h"

namespace textool {

/** greebo: A structure defining a 2D rectangular shape 
 * 			by specifying the top left and the 
 * 			bottom right corner coordinates.
 */
class Rectangle {
public:
	Vector2 topLeft;
	Vector2 bottomRight;
	
	/** greebo: Returns TRUE if the given <point> lies within the
	 * 			boundaries described by the rectangle, FALSE otherwise. 
	 */
	bool contains(const Vector2& point) const {
		return (point[0] >= topLeft[0] && point[0] <= bottomRight[0] && 
				point[1] >= topLeft[1] && point[1] <= bottomRight[1]);
	}
	
	/** greebo: Makes sure that the topLeft vector is actually top left
	 * 			relatively to bottomRight (values should be lower). 
	 */
	void sortCorners() {
		if (topLeft[0] >= bottomRight[0]) {
			std::swap(topLeft[0], bottomRight[0]);
		}
		
		if (topLeft[1] >= bottomRight[1]) {
			std::swap(topLeft[1], bottomRight[1]);
		}
	}
};

} // namespace textool

#endif /*RECTANGLE_H_*/
