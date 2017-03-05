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

	virtual ~FaceItem() {}

	// Gets the AABB of this object in texture space
	virtual AABB getExtents() override;

	// Renders this object's visual representation.
	virtual void render() override;

	// Transforms this object with the given transformation matrix
	virtual void transform(const Matrix4& matrix) override;

    // Overrides default behaviour to pass the call to the first selected child item only
    // otherwise we get a double translation when moving stuff
    virtual void transformSelected(const Matrix4& matrix) override;

	/** greebo: Returns true if the object can be selected at the given coords.
	 */
	virtual bool testSelect(const Rectangle& rectangle) override;

	/** greebo: Snaps the face translation to the grid.
	 */
	virtual void snapSelectedToGrid(float grid) override;

	/** greebo: Flips the face texdef (if selected)
	 * 			about the given axis in texture space.
	 *
	 *  @axis:  0 = s-axis flip, 1 = t-axis flip
	 */
	virtual void flipSelected(const int& axis) override;

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
