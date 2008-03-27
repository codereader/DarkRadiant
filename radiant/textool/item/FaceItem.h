#ifndef FACEITEM_H_
#define FACEITEM_H_

#include "textool/TexToolItem.h"

class Face;
class Winding;

namespace textool {

class FaceItem :
	public TexToolItem
{
	// The patch this control is referring to
	Face& _sourceFace;
	Winding& _winding;

public:
	FaceItem(Face& _sourceFace); 

	// Gets the AABB of this object in texture space
	virtual AABB getExtents();

	// Renders this object's visual representation.
	virtual void render();

	// Transforms this object with the given transformation matrix
	virtual void transform(const Matrix4& matrix);

	/** greebo: Returns true if the object can be selected at the given coords.
	 */
	virtual bool testSelect(const Rectangle& rectangle);
	
	/** greebo: Snaps the face translation to the grid.
	 */
	virtual void snapSelectedToGrid(float grid);
	
	/** greebo: Flips the face texdef (if selected) 
	 * 			about the given axis in texture space.
	 * 
	 *  @axis:  0 = s-axis flip, 1 = t-axis flip		
	 */
	virtual void flipSelected(const int& axis);
	
private:
	/** greebo: Calculates the mean value of all the texCoords,
	 * 			which is technically the centroid.
	 * 
	 * @returns: the Vector2 containing the centroid's coords.
	 */
	Vector2 getCentroid() const;
};
	
} // namespace TexTool

#endif /*FACEITEM_H_*/
