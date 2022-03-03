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

namespace
{

// Prepare a glDraw call by setting the glVertex(Attrib)Pointers to the given starting vertex
inline void setupAttributePointers(ArbitraryMeshVertex* startVertex)
{
    glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &startVertex->vertex);
    glColorPointer(4, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &startVertex->colour);
    glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &startVertex->texcoord);
    glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &startVertex->normal);

    glVertexAttribPointer(GLProgramAttribute::Position, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &startVertex->vertex);
    glVertexAttribPointer(GLProgramAttribute::Normal, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &startVertex->normal);
    glVertexAttribPointer(GLProgramAttribute::TexCoord, 2, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &startVertex->texcoord);
    glVertexAttribPointer(GLProgramAttribute::Tangent, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &startVertex->tangent);
    glVertexAttribPointer(GLProgramAttribute::Bitangent, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &startVertex->bitangent);
}

}

void ObjectRenderer::SubmitGeometry(IGeometryStore::Slot slot, GLenum primitiveMode, IGeometryStore& store)
{
    auto renderParams = store.getRenderParameters(slot);

    setupAttributePointers(renderParams.bufferStart);

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

    setupAttributePointers(bufferStart);

    glMultiDrawElementsBaseVertex(primitiveMode, sizes.data(), GL_UNSIGNED_INT,
        firstIndices.data(), static_cast<GLsizei>(sizes.size()), firstVertices.data());
}

}
