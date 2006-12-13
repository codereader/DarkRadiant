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
	Matrix4 getTransform(float width, float height) const;
	
	void shift(float s, float t);
	void scale(float s, float t);
	void rotate(float angle);

	void normalise(float width, float height);
	
	Matrix4 getBasisForNormal(const Vector3& normal) const;
	
	void transformLocked(std::size_t width, std::size_t height, const Plane3& plane, const Matrix4& identity2transformed);
	
	// Fits a texture to a brush face
	void fitTexture(std::size_t width, std::size_t height, const Vector3& normal, const Winding& w, float s_repeat, float t_repeat);
	
	// greebo: Looks like this method saves the texture definitions into the brush winding points
	void emitTextureCoordinates(std::size_t width, std::size_t height, Winding& w, const Vector3& normal, const Matrix4& localToWorld) const;
	
}; // class TextureProjection

#endif /*TEXTUREPROJECTION_H_*/
