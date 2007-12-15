#ifndef TEXTUREPROJECTION_H_
#define TEXTUREPROJECTION_H_

#include "texturelib.h"
#include "Winding.h"
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

	// Copy Constructor
	TextureProjection(const TextureProjection& other) :
		m_texdef(other.m_texdef),
		m_brushprimit_texdef(other.m_brushprimit_texdef),
		m_basis_s(other.m_basis_s),
		m_basis_t(other.m_basis_t),
		_defaultTextureScale(other._defaultTextureScale)
	{}
	
	void assign(const TextureProjection& other);
	void constructDefault();
		
	void setTransform(float width, float height, const Matrix4& transform);
	Matrix4 getTransform() const;
	
	void shift(float s, float t);
	void scale(float s, float t);
	void rotate(float angle);

	// Normalise projection for a given texture width and height.
	void normalise(float width, float height);
	
	Matrix4 getBasisForNormal(const Vector3& normal) const;
	
	void transformLocked(std::size_t width, std::size_t height, const Plane3& plane, const Matrix4& identity2transformed);
	
	// Fits a texture to a brush face
	void fitTexture(std::size_t width, std::size_t height, const Vector3& normal, const Winding& w, float s_repeat, float t_repeat);
	
	/** greebo: Mirrors the texture around the given axis.
	 * 
	 * @flipAxis: 0 = flip x, 1 = flip y
	 */
	void flipTexture(unsigned int flipAxis);
	
	// greebo: Looks like this method saves the texture definitions into the brush winding points
	void emitTextureCoordinates(Winding& w, const Vector3& normal, const Matrix4& localToWorld) const;
	
	// greebo: This returns a matrix that transforms world vertex coordinates into this texture space
	Matrix4 getWorldToTexture(const Vector3& normal, const Matrix4& localToWorld) const;
	
}; // class TextureProjection

#endif /*TEXTUREPROJECTION_H_*/
