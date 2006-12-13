#include "TextureProjection.h"

// Assigns an <other> projection to this one
void TextureProjection::assign(const TextureProjection& other) {
	m_brushprimit_texdef = other.m_brushprimit_texdef;
}

void TextureProjection::constructDefault() {
	float scale = GlobalRegistry().getFloat("game/defaults/textureScale");
	
	m_texdef._scale[0] = scale;
	m_texdef._scale[1] = scale;

	m_brushprimit_texdef = BrushPrimitTexDef(m_texdef);
}

/* greebo: Uses the transformation matrix <transform> to set the internal texture
 * definitions. Checks the matrix for validity and passes it on to
 * the according internal texture definitions (TexDef or BPTexDef)
 */
void TextureProjection::setTransform(float width, float height, const Matrix4& transform) {
	// Check the matrix for validity
	if ((transform[0] != 0 || transform[4] != 0) && (transform[1] != 0 || transform[5] != 0)) {
		m_brushprimit_texdef = BrushPrimitTexDef(transform);
	} else {
		std::cout << "invalid texture matrix\n";
	}
}

/* greebo: Returns the transformation matrix from the
 * texture definitions members. 
 */
Matrix4 TextureProjection::getTransform(float width, float height) const {
	return m_brushprimit_texdef.getTransform();
}

void TextureProjection::shift(float s, float t) {
	m_brushprimit_texdef.shift(s, t);
}

void TextureProjection::scale(float s, float t) {
	m_brushprimit_texdef.scale(s, t);
}

void TextureProjection::rotate(float angle) {
	m_brushprimit_texdef.rotate(angle);
}

// Normalise projection for a given texture width and height.
void TextureProjection::normalise(float width, float height) {
	m_brushprimit_texdef.normalise(width, height);
}

Matrix4 TextureProjection::getBasisForNormal(const Vector3& normal) const {
	
	Matrix4 basis;
	
	basis = g_matrix4_identity;
	ComputeAxisBase(normal, basis.x().getVector3(), basis.y().getVector3());
	basis.z().getVector3() = normal;
	basis.transpose();
					
	return basis;
}

void TextureProjection::transformLocked(std::size_t width, std::size_t height, const Plane3& plane, const Matrix4& identity2transformed) {
	//globalOutputStream() << "identity2transformed: " << identity2transformed << "\n";

	//globalOutputStream() << "plane.normal(): " << plane.normal() << "\n";

	Vector3 normalTransformed(matrix4_transformed_direction(identity2transformed, plane.normal()));

  //globalOutputStream() << "normalTransformed: " << normalTransformed << "\n";

  // identity: identity space
  // transformed: transformation
  // stIdentity: base st projection space before transformation
  // stTransformed: base st projection space after transformation
  // stOriginal: original texdef space

  // stTransformed2stOriginal = stTransformed -> transformed -> identity -> stIdentity -> stOriginal

	Matrix4 identity2stIdentity = getBasisForNormal(plane.normal());
	//globalOutputStream() << "identity2stIdentity: " << identity2stIdentity << "\n";
	
	Matrix4 transformed2stTransformed = getBasisForNormal(normalTransformed);
	
	Matrix4 stTransformed2identity(matrix4_affine_inverse(matrix4_multiplied_by_matrix4(transformed2stTransformed, identity2transformed)));
	
	Vector3 originalProjectionAxis(matrix4_affine_inverse(identity2stIdentity).z().getVector3());
	
	Vector3 transformedProjectionAxis(stTransformed2identity.z().getVector3());
	
	Matrix4 stIdentity2stOriginal = getTransform((float)width, (float)height);
	Matrix4 identity2stOriginal(matrix4_multiplied_by_matrix4(stIdentity2stOriginal, identity2stIdentity));

	//globalOutputStream() << "originalProj: " << originalProjectionAxis << "\n";
	//globalOutputStream() << "transformedProj: " << transformedProjectionAxis << "\n";
	double dot = originalProjectionAxis.dot(transformedProjectionAxis);
	//globalOutputStream() << "dot: " << dot << "\n";
	if (dot == 0) {
		// The projection axis chosen for the transformed normal is at 90 degrees
		// to the transformed projection axis chosen for the original normal.
		// This happens when the projection axis is ambiguous - e.g. for the plane
		// 'X == Y' the projection axis could be either X or Y.
		//globalOutputStream() << "flipped\n";
#if 0
			globalOutputStream() << "projection off by 90\n";
		globalOutputStream() << "normal: ";
		print_vector3(plane.normal());
		globalOutputStream() << "original projection: ";
		print_vector3(originalProjectionAxis);
		globalOutputStream() << "transformed projection: ";
			print_vector3(transformedProjectionAxis);
#endif

			Matrix4 identityCorrected = matrix4_reflection_for_plane45(plane, originalProjectionAxis, transformedProjectionAxis);

			identity2stOriginal = matrix4_multiplied_by_matrix4(identity2stOriginal, identityCorrected);
		}

		Matrix4 stTransformed2stOriginal = matrix4_multiplied_by_matrix4(identity2stOriginal, stTransformed2identity);

		setTransform((float)width, (float)height, stTransformed2stOriginal);
		normalise((float)width, (float)height);
	}
	
// Fits a texture to a brush face
void TextureProjection::fitTexture(std::size_t width, std::size_t height, const Vector3& normal, const Winding& w, float s_repeat, float t_repeat) {
	if (w.numpoints < 3) {
		return;
	}

	Matrix4 st2tex = getTransform((float)width, (float)height);

	// the current texture transform
	Matrix4 local2tex = st2tex;
	{
		Matrix4 xyz2st; 
		xyz2st = getBasisForNormal(normal);
		matrix4_multiply_by_matrix4(local2tex, xyz2st);
	}

	// the bounds of the current texture transform
	AABB bounds;
	for (Winding::const_iterator i = w.begin(); i != w.end(); ++i) {
		Vector3 texcoord = matrix4_transformed_point(local2tex, i->vertex);
		bounds.includePoint(texcoord);
	}
	bounds.origin.z() = 0;
	bounds.extents.z() = 1;
	
	// the bounds of a perfectly fitted texture transform
	AABB perfect(Vector3(s_repeat * 0.5, t_repeat * 0.5, 0), Vector3(s_repeat * 0.5, t_repeat * 0.5, 1));
	
	// the difference between the current texture transform and the perfectly fitted transform
	Matrix4 matrix = Matrix4::getTranslation(bounds.origin - perfect.origin);
	matrix4_pivoted_scale_by_vec3(matrix, bounds.extents / perfect.extents, perfect.origin);
	matrix4_affine_invert(matrix);
	
	// apply the difference to the current texture transform
	matrix4_premultiply_by_matrix4(st2tex, matrix);
	
	setTransform((float)width, (float)height, st2tex);
	normalise((float)width, (float)height);
}

// greebo: Looks like this method saves the texture definitions into the brush winding points
void TextureProjection::emitTextureCoordinates(std::size_t width, std::size_t height, Winding& w, const Vector3& normal, const Matrix4& localToWorld) const {
	if (w.numpoints < 3) {
		return;
	}
	//globalOutputStream() << "normal: " << normal << "\n";

	Matrix4 local2tex = getTransform((float)width, (float)height);
	//globalOutputStream() << "texdef: " << static_cast<const Vector3&>(local2tex.x()) << static_cast<const Vector3&>(local2tex.y()) << "\n";

	{
		Matrix4 xyz2st; 
		// we don't care if it's not normalised...
		xyz2st = getBasisForNormal(matrix4_transformed_direction(localToWorld, normal));
		//globalOutputStream() << "basis: " << static_cast<const Vector3&>(xyz2st.x()) << static_cast<const Vector3&>(xyz2st.y()) << static_cast<const Vector3&>(xyz2st.z()) << "\n";
		matrix4_multiply_by_matrix4(local2tex, xyz2st);
	}

	Vector3 tangent(local2tex.getTransposed().x().getVector3().getNormalised());
	Vector3 bitangent(local2tex.getTransposed().y().getVector3().getNormalised());
	
	matrix4_multiply_by_matrix4(local2tex, localToWorld);
	
	for (Winding::iterator i = w.begin(); i != w.end(); ++i) {
		Vector3 texcoord = matrix4_transformed_point(local2tex, i->vertex);
		i->texcoord[0] = texcoord[0];
		i->texcoord[1] = texcoord[1];
	
		i->tangent = tangent;
		i->bitangent = bitangent;
	}
}
