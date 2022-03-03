#include "ObjectRenderer.h"

#include "igl.h"
#include "math/Matrix4.h"
#include "igeometrystore.h"
#include "render/ArbitraryMeshVertex.h"

namespace render
{

void ObjectRenderer::SubmitObject(IRenderableObject& object, IGeometryStore& store)
{
    if (object.getObjectTransform().getHandedness() == Matrix4::RIGHTHANDED)
    {
        glFrontFace(GL_CW);
    }
    else
    {
        glFrontFace(GL_CCW);
    }

    // Orient the object
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixd(object.getObjectTransform());

    // Submit the geometry of this single slot (objects are using triangle primitives)
    SubmitGeometry(object.getStorageLocation(), GL_TRIANGLES, store);

    glPopMatrix();
}

void ObjectRenderer::SubmitGeometry(IGeometryStore::Slot slot, GLenum primitiveMode, IGeometryStore& store)
{
    auto renderParams = store.getRenderParameters(slot);

    glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->vertex);
    glColorPointer(4, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->colour);
    glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->normal);
    glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->texcoord);

    glVertexAttribPointer(GLProgramAttribute::Position, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->vertex);
    glVertexAttribPointer(GLProgramAttribute::Normal, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->normal);
    glVertexAttribPointer(GLProgramAttribute::TexCoord, 2, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->texcoord);
    glVertexAttribPointer(GLProgramAttribute::Tangent, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->tangent);
    glVertexAttribPointer(GLProgramAttribute::Bitangent, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->bitangent);

    glDrawElementsBaseVertex(primitiveMode, static_cast<GLsizei>(renderParams.indexCount),
        GL_UNSIGNED_INT, renderParams.firstIndex, static_cast<GLint>(renderParams.firstVertex));
}

void ObjectRenderer::SubmitGeometry(const std::set<IGeometryStore::Slot>& slots, GLenum primitiveMode, IGeometryStore& store)
{
    auto surfaceCount = slots.size();

    if (surfaceCount == 0) return;

    // Build the indices and offsets used for the glMulti draw call
    std::vector<GLsizei> sizes;
    std::vector<void*> firstIndices;
    std::vector<GLint> firstVertices;

    sizes.reserve(surfaceCount);
    firstIndices.reserve(surfaceCount);
    firstVertices.reserve(surfaceCount);

    ArbitraryMeshVertex* bufferStart = nullptr;

    for (const auto slot : slots)
    {
        auto renderParams = store.getRenderParameters(slot);

        sizes.push_back(static_cast<GLsizei>(renderParams.indexCount));
        firstVertices.push_back(static_cast<GLint>(renderParams.firstVertex));
        firstIndices.push_back(renderParams.firstIndex);

        bufferStart = renderParams.bufferStart;
    }

    glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &bufferStart->vertex);
    glColorPointer(4, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &bufferStart->colour);
    glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &bufferStart->texcoord);
    glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &bufferStart->normal);

    glVertexAttribPointer(GLProgramAttribute::Position, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &bufferStart->vertex);
    glVertexAttribPointer(GLProgramAttribute::Normal, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &bufferStart->normal);
    glVertexAttribPointer(GLProgramAttribute::TexCoord, 2, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &bufferStart->texcoord);
    glVertexAttribPointer(GLProgramAttribute::Tangent, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &bufferStart->tangent);
    glVertexAttribPointer(GLProgramAttribute::Bitangent, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &bufferStart->bitangent);

    glMultiDrawElementsBaseVertex(primitiveMode, sizes.data(), GL_UNSIGNED_INT,
        &firstIndices.front(), static_cast<GLsizei>(sizes.size()), &firstVertices.front());
}

}
