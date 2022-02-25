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

    glMatrixMode(GL_MODELVIEW);

    auto renderParams = store.getRenderParameters(object.getStorageLocation());

    glPushMatrix();

    glMultMatrixd(object.getObjectTransform());

    glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->vertex);
    glColorPointer(4, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->colour);

    glVertexAttribPointer(GLProgramAttribute::Position, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->vertex);
    glVertexAttribPointer(GLProgramAttribute::Normal, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->normal);
    glVertexAttribPointer(GLProgramAttribute::TexCoord, 2, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->texcoord);
    glVertexAttribPointer(GLProgramAttribute::Tangent, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->tangent);
    glVertexAttribPointer(GLProgramAttribute::Bitangent, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->bitangent);

    glDrawElementsBaseVertex(GL_TRIANGLES, static_cast<GLsizei>(renderParams.indexCount),
        GL_UNSIGNED_INT, renderParams.firstIndex, static_cast<GLint>(renderParams.firstVertex));

    glPopMatrix();
}

}
