#ifndef FACEITEM_H_
#define FACEITEM_H_

#include "brush/Face.h"
#include "TexToolItem.h"

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
	AABB getExtents();

	// ========== Renderable implementation ================
	
	// Renders this object's visual representation.
	void render();

	// ========== Transformable implementation ================
	
	// Transforms this object with the given transformation matrix
	void transform(const Matrix4& matrix);

	// Transforms this object if it's selected only
	void transformSelected(const Matrix4& matrix);

	// ========== Selectable implementation ================
	
	/** greebo: Returns true if the object can be selected at the given coords.
	 */
	virtual bool testSelect(const Rectangle& rectangle);
	
	/** greebo: Returns the list of selectables at the given coordinates. 
	 */
	virtual TexToolItemVec getSelectables(const Rectangle& rectangle);
	
	/** greebo: Saves the current undo state.
	 */
	virtual void beginTransformation();
	
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
