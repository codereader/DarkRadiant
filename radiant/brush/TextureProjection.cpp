#include "TextureProjection.h"

// Assigns an <other> projection to this one
void TextureProjection::assign(const TextureProjection& other) {
	m_brushprimit_texdef = other.m_brushprimit_texdef;
}

void TextureProjection::constructDefault() {
	float scale = GlobalRegistry().getFloat("user/ui/textures/defaultTextureScale");
	
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
Matrix4 TextureProjection::getTransform() const {
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

/* greebo: This returns the basis vectors of the texture (plane) space.
 * The vectors are normalised and stored within the basis matrix <basis>
 * as line vectors.
 * 
 * Note: the normal vector MUST be normalised already when this function is called,
 * but this should be fulfilled as it represents a FacePlane vector (which is usually normalised)
 */
Matrix4 TextureProjection::getBasisForNormal(const Vector3& normal) const {
	
	Matrix4 basis;
	
	basis = g_matrix4_identity;
	ComputeAxisBase(normal, basis.x().getVector3(), basis.y().getVector3());
	basis.z().getVector3() = normal;
	
	// At this point the basis matrix contains three lines that are
	// perpendicular to each other. 
	
	// The x-line of <basis> contains the <texS> basis vector (within the face plane)
	// The y-line of <basis> contains the <texT> basis vector (within the face plane)
	// The z-line of <basis> contains the <normal> basis vector (perpendicular to the face plane)
	
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
	
	Matrix4 stIdentity2stOriginal = getTransform();
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

	Matrix4 st2tex = getTransform();

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

void TextureProjection::flipTexture(unsigned int flipAxis) {
	// Retrieve the "fake" texture coordinates (shift, scale, rotation)	
	TexDef texdef = m_brushprimit_texdef.getFakeTexCoords();
	
	// Check for x flip (x-component not zero)
	if (flipAxis == 0) {
		// Invert the x scale and rotate 180°
		texdef._scale[0] *= -1;
		texdef._rotate -= 180;
	}
	else if (flipAxis == 1) {
		// Invert the y scale and rotate 180°
		texdef._scale[1] *= -1;
		texdef._rotate -= 180;
	}
	else {
		// Do nothing, leave the brushprimittexdef untouched
		return;
	}
	
	m_brushprimit_texdef = BrushPrimitTexDef(texdef);
}

Matrix4 TextureProjection::getWorldToTexture(const Vector3& normal, const Matrix4& localToWorld) const {
	// Get the transformation matrix, that contains the shift, scale and rotation 
	// of the texture in "condensed" form (as matrix components).
	Matrix4 local2tex = getTransform();

	// Now combine the normal vector with the local2tex matrix
	// to retrieve the final transformation that transforms vertex
	// coordinates into the texture plane.   
	{
		Matrix4 xyz2st; 
		// we don't care if it's not normalised...
		
		// Retrieve the basis vectors of the texture plane space, they are perpendicular to <normal>
		xyz2st = getBasisForNormal(matrix4_transformed_direction(localToWorld, normal));
		
		// Transform the basis vectors with the according texture scale, rotate and shift operations
		// These are contained in the local2tex matrix, so the matrices have to be multiplied. 
		matrix4_multiply_by_matrix4(local2tex, xyz2st);
	}
	
	// Transform the texture basis vectors into the "BrushFace space"
	// usually the localToWorld matrix is identity, so this doesn't do anything.
	matrix4_multiply_by_matrix4(local2tex, localToWorld);
	
	return local2tex;
}

/* greebo: This method calculates the texture coordinates for the brush winding vertices
 * via matrix operations and stores the results into the Winding vertices (together with the
 * tangent and bitangent vectors)
 * 
 * Note: The matrix localToWorld is basically useless at the moment, as it is the identity matrix for faces, and this method
 * gets called on face operations only... */ 
void TextureProjection::emitTextureCoordinates(Winding& w, const Vector3& normal, const Matrix4& localToWorld) const {
	
	// Quit, if we have less than three points (degenerate brushes?) 
	if (w.numpoints < 3) {
		return;
	}
	
	// Get the transformation matrix, that contains the shift, scale and rotation 
	// of the texture in "condensed" form (as matrix components).
	Matrix4 local2tex = getTransform();

	// Now combine the face normal vector with the local2tex matrix
	// to retrieve the final transformation that transforms brush vertex
	// coordinates into the texture plane.   
	{
		Matrix4 xyz2st; 
		// we don't care if it's not normalised...
		
		// Retrieve the basis vectors of the texture plane space, they are perpendicular to <normal>
		xyz2st = getBasisForNormal(matrix4_transformed_direction(localToWorld, normal));
		
		// Transform the basis vectors with the according texture scale, rotate and shift operations
		// These are contained in the local2tex matrix, so the matrices have to be multiplied. 
		matrix4_multiply_by_matrix4(local2tex, xyz2st);
	}
	
	// Calculate the tangent and bitangent vectors to allow the correct openGL transformations
	Vector3 tangent(local2tex.getTransposed().x().getVector3().getNormalised());
	Vector3 bitangent(local2tex.getTransposed().y().getVector3().getNormalised());
	
	// Transform the texture basis vectors into the "BrushFace space"
	// usually the localToWorld matrix is identity, so this doesn't do anything.
	matrix4_multiply_by_matrix4(local2tex, localToWorld);
	
	// Cycle through the winding vertices and apply the texture transformation matrix
	// onto each of them.
	for (Winding::iterator i = w.begin(); i != w.end(); ++i) {
		Vector3 texcoord = local2tex.transform(i->vertex).getVector3();
		
		// Store the s,t coordinates into the winding texcoord vector
		i->texcoord[0] = texcoord[0];
		i->texcoord[1] = texcoord[1];
	
		// Save the tangent and bitangent vectors, they are the same for all the face vertices
		i->tangent = tangent;
		i->bitangent = bitangent;
	}
}
