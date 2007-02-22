#ifndef FACEITEM_H_
#define FACEITEM_H_

#include "TexToolItem.h"

class Face;
class Winding;

namespace selection {
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
	
private:
	/** greebo: Calculates the mean value of all the texCoords,
	 * 			which is technically the centroid.
	 * 
	 * @returns: the Vector2 containing the centroid's coords.
	 */
	Vector2 getCentroid() const;
};
	
	} // namespace TexTool
} // namespace selection

#endif /*FACEITEM_H_*/
