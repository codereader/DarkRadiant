#pragma once

#include "TargetKeyCollection.h"
#include "render.h"
#include "irenderable.h"
#include "ivolumetest.h"
#include "math/Segment.h"

namespace entity 
{

namespace
{
    const double TARGET_MAX_ARROW_LENGTH = 10;
}

/**
 * greebo: This is a helper object owned by the TargetableInstance.
 * It represents a RenderablePointVector which repopulates
 * itself each frame with the coordinates of the targeted
 * instances. It provides a render() method.
 *
 * The render() method is invoked by the TargetableNode during the
 * frontend render pass.
 */
class RenderableTargetLines :
	public RenderablePointVector
{
	const TargetKeyCollection& _targetKeys;

public:
	RenderableTargetLines(const TargetKeyCollection& targetKeys) :
		RenderablePointVector(GL_LINES),
		_targetKeys(targetKeys)
	{}

    bool hasTargets() const
    {
        return !_targetKeys.empty();
    }

	void render(const ShaderPtr& shader, RenderableCollector& collector, const VolumeTest& volume, const Vector3& worldPosition)
	{
		if (_targetKeys.empty())
		{
			return;
		}

		// Clear the vector
		clear();

		// Populate the RenderablePointVector with all target coordinates
        _targetKeys.forEachTarget([&] (const TargetPtr& target)
        {
            if (!target || target->isEmpty() || !target->isVisible())
            {
                return;
            }

            Vector3 targetPosition = target->getPosition();

            if (volume.TestLine(Segment::createForStartEnd(worldPosition, targetPosition)))
            {
                addTargetLine(worldPosition, targetPosition);
            }
        });

		// If we hold any objects now, add us as renderable
		if (!empty())
        {
			collector.addRenderable(shader, *this, Matrix4::getIdentity());
		}
	}

private:
    // Adds points to the vector, defining a line from start to end, with arrow indicators
    // in the XY plane (located at the midpoint between start/end).
    void addTargetLine(const Vector3& startPosition, const Vector3& endPosition)
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

        // The line from this to the other entity
        push_back(VertexCb(startPosition));
        push_back(VertexCb(endPosition));

        // The "arrow indicators" in the xy plane
        push_back(VertexCb(mid));
        push_back(VertexCb(xyPoint1));

        push_back(VertexCb(mid));
        push_back(VertexCb(xyPoint2));
    }
};

} // namespace entity
