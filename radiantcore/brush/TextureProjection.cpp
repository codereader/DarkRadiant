#include "TextureProjection.h"

#include "registry/CachedKey.h"
#include "texturelib.h"
#include "itextstream.h"
#include <limits>

TextureProjection::TextureProjection() :
    TextureProjection(GetDefaultProjection())
{}

TextureProjection::TextureProjection(const TextureProjection& other) :
    TextureProjection(other._matrix)
{}

TextureProjection::TextureProjection(const TextureMatrix& otherMatrix) :
    _matrix(otherMatrix)
{}

const TextureMatrix& TextureProjection::getTextureMatrix() const
{
    return _matrix;
}

TextureMatrix& TextureProjection::getTextureMatrix()
{
    return _matrix;
}

TextureMatrix TextureProjection::GetDefaultProjection()
{
    // Cache the registry key because this constructor is called a lot
    static registry::CachedKey<float> scaleKey(
        "user/ui/textures/defaultTextureScale"
    );

    TexDef tempTexDef;

    double scale = scaleKey.get();
    tempTexDef.setScale(Vector2(scale, scale));

    return TextureMatrix(tempTexDef);
}

// Assigns an <other> projection to this one
void TextureProjection::assign(const TextureProjection& other)
{
    _matrix = other._matrix;
}

void TextureProjection::setTransform(const Matrix3& transform)
{
    // Check the matrix for validity
    if ((transform.xx() != 0 || transform.yx() != 0) && (transform.xy() != 0 || transform.yy() != 0))
    {
        _matrix = TextureMatrix(transform);
    }
    else
    {
        rError() << "invalid texture matrix" << std::endl;
    }
}

void TextureProjection::setTransformFromMatrix4(const Matrix4& transform)
{
    setTransform(getTextureMatrixFromMatrix4(transform));
}

void TextureProjection::setFromTexDef(const TexDef& texDef)
{
    _matrix = TextureMatrix(texDef);
}

Matrix4 TextureProjection::getMatrix4() const
{
    return getMatrix4FromTextureMatrix(_matrix.getMatrix3());
}

Matrix3 TextureProjection::getMatrix() const
{
    return _matrix.getMatrix3();
}

void TextureProjection::shift(double s, double t)
{
    _matrix.shift(s, t);
}

void TextureProjection::normalise(float width, float height)
{
    _matrix.normalise(width, height);
}

// Fits a texture to a brush face
void TextureProjection::fitTexture(std::size_t width, std::size_t height,
                                   const Vector3& normal, const Winding& winding,
                                   float s_repeat, float t_repeat)
{
    if (winding.size() < 3)
    {
        return;
    }

    // Sanity-check the matrix, if it contains any NaNs or INFs we fall back to the default projection (#5371)
    Matrix4 st2tex = _matrix.isSane() ? getMatrix4() : GetDefaultProjection().getTransform();

    // the current texture transform
    Matrix4 local2tex = st2tex;
    {
        Matrix4 xyz2st = getBasisTransformForNormal(normal);
        local2tex.multiplyBy(xyz2st);
    }

    // the bounds of the current texture transform
    AABB bounds;
    for (const auto& vertex : winding)
    {
        bounds.includePoint(local2tex.transformPoint(vertex.vertex));
    }
    bounds.origin.z() = 0;
    bounds.extents.z() = 1;

    // the bounds of a perfectly fitted texture transform
    AABB perfect(Vector3(s_repeat * 0.5, t_repeat * 0.5, 0),
                 Vector3(s_repeat * 0.5, t_repeat * 0.5, 1));

    // the difference between the current texture transform and the perfectly fitted transform
    Matrix4 diffMatrix = Matrix4::getTranslation(bounds.origin - perfect.origin);
	diffMatrix.scaleBy(bounds.extents / perfect.extents, perfect.origin);
	diffMatrix.invert();

    // apply the difference to the current texture transform
    st2tex.premultiplyBy(diffMatrix);

    setTransformFromMatrix4(st2tex);
    normalise((float)width, (float)height);
}

void TextureProjection::alignTexture(IFace::AlignEdge align, const Winding& winding)
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
    case IFace::AlignEdge::Top:
        windingIndex = topEdge;
        dim = 1;
        break;
    case IFace::AlignEdge::Bottom:
        windingIndex = bottomEdge;
        dim = 1;
        break;
    case IFace::AlignEdge::Left:
        windingIndex = leftEdge;
        dim = 0;
        break;
    case IFace::AlignEdge::Right:
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

Matrix4 TextureProjection::getWorldToTexture(const Vector3& normal, const Matrix4& localToWorld) const
{
    // See the emitTextureCoordinates() method for more comments on these transformation steps

    // Texture Projection
    auto local2tex = getMatrix4();

    // Axis Base
    auto xyz2st = getBasisTransformForNormal(localToWorld.transformDirection(normal));
    local2tex.multiplyBy(xyz2st);

    // L2W (usually an identity transform)
    local2tex.multiplyBy(localToWorld);

    return local2tex;
}

void TextureProjection::emitTextureCoordinates(Winding& winding, const Vector3& normal, const Matrix4& localToWorld) const
{
    // Quit if we have less than three points (degenerate brushes?)
    if (winding.size() < 3) return;

    // Load the 2D texture projection matrix, transforming XY coordinates to UV
    auto local2tex = getMatrix4();

    // Using the face's normal we can construct a rotation transformation,
    // which rotates world space such that the Z axis is aligned with the face normal.
    // After this stage we only need to consider the XY part of the 3D vertices.
    auto xyz2st = getBasisTransformForNormal(localToWorld.transformDirection(normal));

    // Transform the basis vectors with the according texture scale, rotate and shift operations
    // These are contained in the local2tex matrix, so the matrices have to be multiplied.
    local2tex.multiplyBy(xyz2st);

    // Calculate the tangent and bitangent vectors to allow the correct openGL transformations
    auto tangent(local2tex.getTransposed().xCol().getVector3().getNormalised());
    auto bitangent(local2tex.getTransposed().yCol().getVector3().getNormalised());

    // Transform the texture basis vectors into the "BrushFace space"
    // usually the localToWorld matrix is identity, so this doesn't do anything.
    local2tex.multiplyBy(localToWorld);

    // Cycle through the winding vertices and apply the texture transformation matrix
    // onto each of them.
    for (auto& vertex : winding)
    {
        auto texcoord = local2tex.transformPoint(vertex.vertex);

        // Store the s,t coordinates into the winding texcoord vector
        vertex.texcoord[0] = texcoord[0];
        vertex.texcoord[1] = texcoord[1];

        // Save the tangent and bitangent vectors, they are the same for all the face vertices
        vertex.tangent = tangent;
        vertex.bitangent = bitangent;
    }
}

Vector2 TextureProjection::getTextureCoordsForVertex(const Vector3& point, const Vector3& normal, const Matrix4& localToWorld) const
{
    auto local2tex = getWorldToTexture(normal, localToWorld);
    
    auto texcoord = local2tex.transformPoint(point);

    return { texcoord.x(), texcoord.y() };
}

void TextureProjection::calculateFromPoints(const Vector3 points[3], const Vector2 uvs[3], const Vector3& normal)
{
    // Calculate the texture projection for the desired set of UVs and XYZ

    // The texture projection matrix is applied to the vertices after they have been
    // transformed by the axis base transform (which depends on this face's normal):
    // T * AB * vertex = UV
    // 
    // Applying AB to the vertices will yield: T * P = texcoord
    // with P containing the axis-based transformed vertices.
    // 
    // If the above should be solved for T, expanding the above multiplication 
    // sets up six equations to calculate the 6 unknown components of T.
    // 
    // We can arrange the 6 equations in matrix form: T * A = B
    // T is the 3x3 texture matrix.
    // A contains the XY coords in its columns (Z is ignored since we 
    // applied the axis base), B contains the UV coords in its columns.
    // The third component of all columns in both matrices is 1.
    // 
    // We can solve the above by inverting A: T = B * inv(A)

    // Get the axis base for this face, we need the XYZ points in that state
    // to reverse-calculate the desired texture transform
    auto axisBase = getBasisTransformForNormal(normal);

    // Rotate the three incoming world vertices into the local face plane
    Vector3 localPoints[] =
    {
        axisBase * points[0],
        axisBase * points[1],
        axisBase * points[2],
    };

    // Arrange the XYZ coords into the columns of matrix A
    auto xyz = Matrix3::byColumns(localPoints[0].x(), localPoints[0].y(), 1,
        localPoints[1].x(), localPoints[1].y(), 1,
        localPoints[2].x(), localPoints[2].y(), 1);

    auto uv = Matrix3::byColumns(uvs[0].x(), uvs[0].y(), 1,
        uvs[1].x(), uvs[1].y(), 1,
        uvs[2].x(), uvs[2].y(), 1);

    setTransform(uv * xyz.getFullInverse());
}
