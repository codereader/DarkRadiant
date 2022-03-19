#pragma once

#include "TargetKeyCollection.h"
#include "render.h"
#include "irenderable.h"
#include "ivolumetest.h"
#include "math/Segment.h"
#include "render/RenderableGeometry.h"

namespace entity
{

namespace
{
    constexpr const double TARGET_MAX_ARROW_LENGTH = 10;
}

/**
 * greebo: This is a helper object owned by the TargetableInstance.
 * It sets up a line-based renderable which repopulates
 * itself each frame with the coordinates of the targeted
 * instances. It provides a render() method.
 *
 * The render() method is invoked by the TargetableNode during the
 * frontend render pass.
 */
class RenderableTargetLines :
    public render::RenderableGeometry
{
private:
    const IEntityNode& _entity;
    const TargetKeyCollection& _targetKeys;

    Vector3 _worldPosition;

    bool _updateNeeded;

public:
    RenderableTargetLines(const IEntityNode& entity, const TargetKeyCollection& targetKeys) :
        _entity(entity),
        _targetKeys(targetKeys),
        _updateNeeded(true)
    {}

    void queueUpdate()
    {
        _updateNeeded = true;
    }

    bool hasTargets() const
    {
        return !_targetKeys.empty();
    }

    void update(const ShaderPtr& shader, const Vector3& worldPosition)
    {
        // Force an update on position change
        _updateNeeded |= worldPosition != _worldPosition;

        if (!_updateNeeded) return;
        
        _updateNeeded = false;

        // Store the new world position for use in updateGeometry()
        _worldPosition = worldPosition;

        // Tell the base class to run the rest of the update routine
        RenderableGeometry::update(shader);
    }

protected:
    void updateGeometry() override
    {
        // Target lines are visible if both their start and end entities are visible
        // This is hard to track in the scope of this class, so we fall back to doing
        // an update on the renderable geometry every time we're asked to

        // Collect vertex and index data
        std::vector<render::RenderVertex> vertices;
        std::vector<unsigned int> indices;
        auto maxTargets = _targetKeys.getNumTargets();

        vertices.reserve(6 * maxTargets);
        indices.reserve(6 * maxTargets);

        _targetKeys.forEachTarget([&](const TargetPtr& target)
        {
            if (!target || target->isEmpty() || !target->isVisible())
            {
                return;
            }

            auto targetPosition = target->getPosition();

            addTargetLine(_worldPosition, targetPosition, vertices, indices);
        });

        updateGeometryWithData(render::GeometryType::Lines, vertices, indices);
    }

private:
    // Adds points to the vector, defining a line from start to end, with arrow indicators
    // in the XY plane (located at the midpoint between start/end).
    void addTargetLine(const Vector3& startPosition, const Vector3& endPosition,
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
};

} // namespace entity
