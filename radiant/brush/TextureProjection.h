#ifndef TEXTUREPROJECTION_H_
#define TEXTUREPROJECTION_H_

#include "texturelib.h"
#include "winding.h"
#include "math/aabb.h"
#include "iregistry.h"
#include "BrushPrimitTexDef.h"

/* greebo: A texture projection contains the texture definition
 * as well as the brush primitive texture definition. 
 */
class TextureProjection {
public:
	TexDef m_texdef;
	BrushPrimitTexDef m_brushprimit_texdef;
	Vector3 m_basis_s;
	Vector3 m_basis_t;
	float _defaultTextureScale;

	// Constructor
	TextureProjection() {}
	
	// Copy Constructor
	TextureProjection(
		const TexDef& texdef,
		const BrushPrimitTexDef& brushprimit_texdef,
		const Vector3& basis_s,
		const Vector3& basis_t
	) :
		m_texdef(texdef),
		m_brushprimit_texdef(brushprimit_texdef),
		m_basis_s(basis_s),
		m_basis_t(basis_t)
	{}
	
	void assign(const TextureProjection& other);
	void constructDefault();
		
	void setTransform(float width, float height, const Matrix4& transform);
	Matrix4 getTransform() const;
	
	void shift(float s, float t);
	void scale(float s, float t);
	void rotate(float angle);

	void normalise(float width, float height);
	
	Matrix4 getBasisForNormal(const Vector3& normal) const;
	
	void transformLocked(std::size_t width, std::size_t height, const Plane3& plane, const Matrix4& identity2transformed);
	
	// Fits a texture to a brush face
	void fitTexture(std::size_t width, std::size_t height, const Vector3& normal, const Winding& w, float s_repeat, float t_repeat);
	
	/** greebo: Mirrors the texture around the given axis.
	 * 
	 * @parameter: If the first component (x-component) of the vector is not 0, the texture is flipped
	 * around the x-axis. Same goes for the y-component (a value unequal to 0 flips it around y).
	 * The z-component is ignored.
	 * 
	 * @flipAxis: Pass <1,0,0> to flipX, <0,1,0> to flipY (haven't tested what happens, if <1,1,0> is passed.
	 */
	void flipTexture(const Vector3& flipAxis);
	
	// greebo: Looks like this method saves the texture definitions into the brush winding points
	void emitTextureCoordinates(Winding& w, const Vector3& normal, const Matrix4& localToWorld) const;
	
	// greebo: This returns a matrix that transforms world vertex coordinates into this texture space
	Matrix4 getWorldToTexture(const Vector3& normal, const Matrix4& localToWorld) const;
	
}; // class TextureProjection

#endif /*TEXTUREPROJECTION_H_*/
