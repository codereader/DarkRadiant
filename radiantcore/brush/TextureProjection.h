#pragma once

#include "ibrush.h"
#include "math/Matrix3.h"
#include "texturelib.h"
#include "Winding.h"
#include "math/AABB.h"
#include "iregistry.h"
#include "TextureMatrix.h"
#include "selection/algorithm/Shader.h"

/* greebo: A texture projection houses the 6 floating points
   necessary to project world coords to texture space.
 */
class TextureProjection
{
public:
    TextureMatrix matrix;

    /**
     * \brief
     * Construct a default TextureProjection.
     *
     * The projection is initialised with the default texture scale from the
     * registry.
     */
    TextureProjection();

    // Copy Constructor
    TextureProjection(const TextureProjection& other);

    // Construct using an existing texture matrix
    TextureProjection(const TextureMatrix& otherMatrix);

    static TextureMatrix GetDefaultProjection();

    void assign(const TextureProjection& other);

    void setTransform(const Matrix3& transform);
    void setTransform(const Matrix4& transform);
    Matrix4 getTransform() const;

	// s and t are texture coordinates, not pixels
    void shift(double s, double t);

    // Normalise projection for a given texture width and height.
    void normalise(float width, float height);

    void transformLocked(std::size_t width, std::size_t height, const Plane3& plane, const Matrix4& identity2transformed);

    // Fits a texture to a brush face
    void fitTexture(std::size_t width, std::size_t height, const Vector3& normal, const Winding& w, float s_repeat, float t_repeat);

    // Aligns this texture to the given edge of the winding
    void alignTexture(IFace::AlignEdge align, const Winding& winding);

    // greebo: Saves the texture definitions into the brush winding points
    void emitTextureCoordinates(Winding& w, const Vector3& normal, const Matrix4& localToWorld) const;

    // greebo: This returns a matrix transforming world vertex coordinates into texture space
    Matrix4 getWorldToTexture(const Vector3& normal, const Matrix4& localToWorld) const;

}; // class TextureProjection
