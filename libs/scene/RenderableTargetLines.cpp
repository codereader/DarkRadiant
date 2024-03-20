#include "RenderableTargetLines.h"
#include "EntityNode.h"

void RenderableTargetLines::addTargetLine(const Vector3& startPosition, const Vector3& endPosition,
    std::vector<render::RenderVertex>& vertices, std::vector<unsigned int>& indices)
{
    // Take the mid-point
    Vector3 mid((startPosition + endPosition) * 0.5f);

    // Get the normalised target direction
    Vector3 targetDir = (endPosition - startPosition);

    // Normalise the length manually to get the scale for the arrows
    double length = targetDir.getLength();
    targetDir *= 1 / length;

    // Get the orthogonal direction (in the xy plane)
    Vector3 xyDir(endPosition.y() - startPosition.y(), startPosition.x() - endPosition.x(), 0);
    xyDir.normalise();

    // Let the target arrow not be any longer than one tenth of the total distance
    double targetArrowLength = length * 0.10f;

    // Clamp the length to a few units anyway
    if (targetArrowLength > TARGET_MAX_ARROW_LENGTH) {
        targetArrowLength = TARGET_MAX_ARROW_LENGTH;
    }

    targetDir *= targetArrowLength;
    xyDir *= targetArrowLength;

    // Get a point slightly away from the target
    Vector3 arrowBase(mid - targetDir);

    // The arrow points for the XY plane
    Vector3 xyPoint1 = arrowBase + xyDir;
    Vector3 xyPoint2 = arrowBase - xyDir;

    auto colour = _entity.getEntityColour();

    auto indexOffset = static_cast<unsigned int>(vertices.size());

    // The line from this to the other entity
    vertices.push_back(render::RenderVertex(startPosition, { 1,0,0 }, { 0, 0 }, colour));
    vertices.push_back(render::RenderVertex(endPosition, { 1,0,0 }, { 0, 0 }, colour));

    // The "arrow indicators" in the xy plane
    vertices.push_back(render::RenderVertex(mid, { 1,0,0 }, { 0, 0 }, colour));
    vertices.push_back(render::RenderVertex(xyPoint1, { 1,0,0 }, { 0, 0 }, colour));

    vertices.push_back(render::RenderVertex(mid, { 1,0,0 }, { 0, 0 }, colour));
    vertices.push_back(render::RenderVertex(xyPoint2, { 1,0,0 }, { 0, 0 }, colour));

    indices.push_back(indexOffset + 0);
    indices.push_back(indexOffset + 1);
    indices.push_back(indexOffset + 2);
    indices.push_back(indexOffset + 3);
    indices.push_back(indexOffset + 4);
    indices.push_back(indexOffset + 5);
}
