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

    // Submit the geometry of this single slot
    SubmitGeometry(object.getStorageLocation(), store);

    glPopMatrix();
}

void ObjectRenderer::SubmitGeometry(IGeometryStore::Slot slot, IGeometryStore& store)
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

    glDrawElementsBaseVertex(GL_TRIANGLES, static_cast<GLsizei>(renderParams.indexCount),
        GL_UNSIGNED_INT, renderParams.firstIndex, static_cast<GLint>(renderParams.firstVertex));
}

}
