#ifndef TEXTOOL_TRANSFORMABLE_H_
#define TEXTOOL_TRANSFORMABLE_H_

class Matrix4;

namespace selection {
	namespace textool {

class Transformable
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
	virtual void transform(const Matrix4& transform) = 0;
};

	} // namespace TexTool
} // namespace selection

#endif /*TEXTOOL_TRANSFORMABLE_H_*/
