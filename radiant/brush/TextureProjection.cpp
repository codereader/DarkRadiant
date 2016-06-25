#include "TextureProjection.h"

#include "registry/CachedKey.h"
#include "texturelib.h"
#include "itextstream.h"
#include <limits>

TextureProjection::TextureProjection() :
    TextureProjection(GetDefaultProjection())
{}

TextureProjection::TextureProjection(const TextureProjection& other) :
    TextureProjection(other.matrix)
{}

TextureProjection::TextureProjection(const TextureMatrix& otherMatrix) :
    matrix(otherMatrix)
{}

TextureMatrix TextureProjection::GetDefaultProjection()
{
    // Cache the registry key because this constructor is called a lot
    static registry::CachedKey<float> scale(
        "user/ui/textures/defaultTextureScale"
    );

    TexDef tempTexDef;

    tempTexDef._scale[0] = scale.get();
    tempTexDef._scale[1] = scale.get();

    return TextureMatrix(tempTexDef);
}

// Assigns an <other> projection to this one
void TextureProjection::assign(const TextureProjection& other)
{
    matrix = other.matrix;
}

/* greebo: Uses the transformation matrix <transform> to set the internal texture
 * definitions. Checks the matrix for validity and passes it on to
 * the according internal texture definitions (TexDef or BPTexDef)
 */
void TextureProjection::setTransform(float width, float height, const Matrix4& transform) {
    // Check the matrix for validity
    if ((transform[0] != 0 || transform[4] != 0) && (transform[1] != 0 || transform[5] != 0)) {
        matrix = TextureMatrix(transform);
    } else {
        rConsole() << "invalid texture matrix" << std::endl;
    }
}

/* greebo: Returns the transformation matrix from the
 * texture definitions members.
 */
Matrix4 TextureProjection::getTransform() const {
    return matrix.getTransform();
}

void TextureProjection::shift(float s, float t)
{
    matrix.shift(s, t);
}

void TextureProjection::scale(float s, float t, std::size_t shaderWidth, std::size_t shaderHeight)
{
    matrix.scale(s, t, shaderWidth, shaderHeight);
}

void TextureProjection::rotate(float angle, std::size_t shaderWidth, std::size_t shaderHeight)
{
    matrix.rotate(angle, shaderWidth, shaderHeight);
}

// Normalise projection for a given texture width and height.
void TextureProjection::normalise(float width, float height) {
    matrix.normalise(width, height);
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

    basis = Matrix4::getIdentity();
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
    //rMessage() << "identity2transformed: " << identity2transformed << "\n";

    //rMessage() << "plane.normal(): " << plane.normal() << "\n";

    Vector3 normalTransformed(identity2transformed.transformDirection(plane.normal()));

  //rMessage() << "normalTransformed: " << normalTransformed << "\n";

  // identity: identity space
  // transformed: transformation
  // stIdentity: base st projection space before transformation
  // stTransformed: base st projection space after transformation
  // stOriginal: original texdef space

  // stTransformed2stOriginal = stTransformed -> transformed -> identity -> stIdentity -> stOriginal

    Matrix4 identity2stIdentity = getBasisForNormal(plane.normal());
    //rMessage() << "identity2stIdentity: " << identity2stIdentity << "\n";

    Matrix4 transformed2stTransformed = getBasisForNormal(normalTransformed);

    Matrix4 stTransformed2identity(
        transformed2stTransformed.getMultipliedBy(identity2transformed).getInverse()
    );

    Vector3 originalProjectionAxis(
        identity2stIdentity.getInverse().z().getVector3()
    );

    Vector3 transformedProjectionAxis(stTransformed2identity.z().getVector3());

    Matrix4 stIdentity2stOriginal = getTransform();
    Matrix4 identity2stOriginal = stIdentity2stOriginal.getMultipliedBy(identity2stIdentity);

    //rMessage() << "originalProj: " << originalProjectionAxis << "\n";
    //rMessage() << "transformedProj: " << transformedProjectionAxis << "\n";
    double dot = originalProjectionAxis.dot(transformedProjectionAxis);
    //rMessage() << "dot: " << dot << "\n";
    if (dot == 0) {
        // The projection axis chosen for the transformed normal is at 90 degrees
        // to the transformed projection axis chosen for the original normal.
        // This happens when the projection axis is ambiguous - e.g. for the plane
        // 'X == Y' the projection axis could be either X or Y.
        //rMessage() << "flipped\n";
#if 0
            rMessage() << "projection off by 90\n";
        rMessage() << "normal: ";
        print_vector3(plane.normal());
        rMessage() << "original projection: ";
        print_vector3(originalProjectionAxis);
        rMessage() << "transformed projection: ";
            print_vector3(transformedProjectionAxis);
#endif

            Matrix4 identityCorrected = matrix4_reflection_for_plane45(plane, originalProjectionAxis, transformedProjectionAxis);

            identity2stOriginal = identity2stOriginal.getMultipliedBy(identityCorrected);
        }

        Matrix4 stTransformed2stOriginal = identity2stOriginal.getMultipliedBy(stTransformed2identity);

        setTransform((float)width, (float)height, stTransformed2stOriginal);
        normalise((float)width, (float)height);
    }

// Fits a texture to a brush face
void TextureProjection::fitTexture(std::size_t width, std::size_t height, const Vector3& normal, const Winding& w, float s_repeat, float t_repeat) {
    if (w.size() < 3) {
        return;
    }

    Matrix4 st2tex = getTransform();

    // the current texture transform
    Matrix4 local2tex = st2tex;
    {
        Matrix4 xyz2st;
        xyz2st = getBasisForNormal(normal);
        local2tex.multiplyBy(xyz2st);
    }

    // the bounds of the current texture transform
    AABB bounds;
    for (Winding::const_iterator i = w.begin(); i != w.end(); ++i) {
        Vector3 texcoord = local2tex.transformPoint(i->vertex);
        bounds.includePoint(texcoord);
    }
    bounds.origin.z() = 0;
    bounds.extents.z() = 1;

    // the bounds of a perfectly fitted texture transform
    AABB perfect(Vector3(s_repeat * 0.5, t_repeat * 0.5, 0), Vector3(s_repeat * 0.5, t_repeat * 0.5, 1));

    // the difference between the current texture transform and the perfectly fitted transform
    Matrix4 diffMatrix = Matrix4::getTranslation(bounds.origin - perfect.origin);
	diffMatrix.scaleBy(bounds.extents / perfect.extents, perfect.origin);
	diffMatrix.invert();

    // apply the difference to the current texture transform
    st2tex.premultiplyBy(diffMatrix);

    setTransform((float)width, (float)height, st2tex);
    normalise((float)width, (float)height);
}

void TextureProjection::flipTexture(unsigned int flipAxis) {
    // Retrieve the "fake" texture coordinates (shift, scale, rotation)
    TexDef texdef = matrix.getFakeTexCoords();

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
        // Do nothing, leave the TextureMatrix untouched
        return;
    }

    matrix = TextureMatrix(texdef);
}

void TextureProjection::alignTexture(EAlignType align, const Winding& winding)
{
    if (winding.empty()) return;

    // The edges in texture space, sorted the same as in the winding
    std::vector<Vector2> texEdges(winding.size());

    // Calculate all edges in texture space
    for (std::size_t i = 0, j = 1; i < winding.size(); ++i, j = winding.next(j))
    {
        texEdges[i] = winding[j].texcoord - winding[i].texcoord;
    }

    // Find the edge which is nearest to the s,t base vector, to classify them as "top" or "left"
    std::size_t bottomEdge = findBestEdgeForDirection(Vector2(1,0), texEdges);
    std::size_t leftEdge = findBestEdgeForDirection(Vector2(0,1), texEdges);
    std::size_t rightEdge = findBestEdgeForDirection(Vector2(0,-1), texEdges);
    std::size_t topEdge = findBestEdgeForDirection(Vector2(-1,0), texEdges);

    // The bottom edge is the one with the larger T texture coordinate
    if (winding[topEdge].texcoord.y() > winding[bottomEdge].texcoord.y())
    {
        std::swap(topEdge, bottomEdge);
    }

    // The right edge is the one with the larger S texture coordinate
    if (winding[rightEdge].texcoord.x() < winding[leftEdge].texcoord.x())
    {
        std::swap(rightEdge, leftEdge);
    }

    // Find the winding vertex index we're calculating the delta for
    std::size_t windingIndex = 0;
    // The dimension to move (1 for top/bottom, 0 for left right)
    std::size_t dim = 0;

    switch (align)
    {
    case ALIGN_TOP:
        windingIndex = topEdge;
        dim = 1;
        break;
    case ALIGN_BOTTOM:
        windingIndex = bottomEdge;
        dim = 1;
        break;
    case ALIGN_LEFT:
        windingIndex = leftEdge;
        dim = 0;
        break;
    case ALIGN_RIGHT:
        windingIndex = rightEdge;
        dim = 0;
        break;
    };

    Vector2 snapped = winding[windingIndex].texcoord;

    // Snap the dimension we're going to change only (s for left/right, t for top/bottom)
    snapped[dim] = float_snapped(snapped[dim], 1.0);

    Vector2 delta = snapped - winding[windingIndex].texcoord;

    // Shift the texture such that we hit the snapped coordinate
    // be sure to invert the s coordinate
    shift(-delta.x(), delta.y());
}

Matrix4 TextureProjection::getWorldToTexture(const Vector3& normal, const Matrix4& localToWorld) const {
    // Get the transformation matrix, that contains the shift, scale and rotation
    // of the texture in "condensed" form (as matrix components).
    Matrix4 local2tex = getTransform();

    // Now combine the normal vector with the local2tex matrix
    // to retrieve the final transformation that transforms vertex
    // coordinates into the texture plane.
    {
        // we don't care if it's not normalised...

        // Retrieve the basis vectors of the texture plane space, they are perpendicular to <normal>
        Matrix4 xyz2st = getBasisForNormal(localToWorld.transformDirection(normal));

        // Transform the basis vectors with the according texture scale, rotate and shift operations
        // These are contained in the local2tex matrix, so the matrices have to be multiplied.
        local2tex.multiplyBy(xyz2st);
    }

    // Transform the texture basis vectors into the "BrushFace space"
    // usually the localToWorld matrix is identity, so this doesn't do anything.
    local2tex.multiplyBy(localToWorld);

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
    if (w.size() < 3) {
        return;
    }

    // Get the transformation matrix, that contains the shift, scale and rotation
    // of the texture in "condensed" form (as matrix components).
    Matrix4 local2tex = getTransform();

    // Now combine the face normal vector with the local2tex matrix
    // to retrieve the final transformation that transforms brush vertex
    // coordinates into the texture plane.
    {
        // we don't care if it's not normalised...

        // Retrieve the basis vectors of the texture plane space, they are perpendicular to <normal>
        Matrix4 xyz2st = getBasisForNormal(localToWorld.transformDirection(normal));

        // Transform the basis vectors with the according texture scale, rotate and shift operations
        // These are contained in the local2tex matrix, so the matrices have to be multiplied.
        local2tex.multiplyBy(xyz2st);
    }

    // Calculate the tangent and bitangent vectors to allow the correct openGL transformations
    Vector3 tangent(local2tex.getTransposed().x().getVector3().getNormalised());
    Vector3 bitangent(local2tex.getTransposed().y().getVector3().getNormalised());

    // Transform the texture basis vectors into the "BrushFace space"
    // usually the localToWorld matrix is identity, so this doesn't do anything.
    local2tex.multiplyBy(localToWorld);

    // Cycle through the winding vertices and apply the texture transformation matrix
    // onto each of them.
    for (Winding::iterator i = w.begin(); i != w.end(); ++i)
    {
        Vector3 texcoord = local2tex.transformPoint(i->vertex);

        // Store the s,t coordinates into the winding texcoord vector
        i->texcoord[0] = texcoord[0];
        i->texcoord[1] = texcoord[1];

        // Save the tangent and bitangent vectors, they are the same for all the face vertices
        i->tangent = tangent;
        i->bitangent = bitangent;
    }
}
