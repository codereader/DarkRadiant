#ifndef _ENTITY_RENDERABLE_TARGETLINES_H_
#define _ENTITY_RENDERABLE_TARGETLINES_H_

#include "TargetKeyCollection.h"
#include "render.h"
#include "irenderable.h"
#include "ivolumetest.h"
#include "math/Segment.h"

namespace entity {

	const double TARGET_MAX_ARROW_LENGTH = 10;

/**
 * greebo: Small utility walker which populates the given pointvector
 *         with the target coordinates while visiting each target.
 */
class TargetLinesPopulator :
	public TargetKeyCollection::Visitor
{
	// The target point vector to populate
	RenderablePointVector& _pointVector;

	// The starting position
	const Vector3& _worldPosition;

	// The volume intersection test
	const VolumeTest& _volume;
public:
	TargetLinesPopulator(RenderablePointVector& pointVector,
						 const Vector3& worldPosition,
						 const VolumeTest& volume) :
		_pointVector(pointVector),
		_worldPosition(worldPosition),
		_volume(volume)
	{}

	virtual void visit(const TargetPtr& target) {
		if (target->isEmpty() || !target->isVisible()) {
			return;
		}

		Vector3 targetPosition = target->getPosition();

		if (_volume.TestLine(Segment::createForStartEnd(_worldPosition, targetPosition)))
		{
			// Take the mid-point
			Vector3 mid((_worldPosition + targetPosition) * 0.5f);

			// Get the normalised target direction
			Vector3 targetDir = (targetPosition - _worldPosition);

			// Normalise the length manually to get the scale for the arrows
			double length = targetDir.getLength();
			targetDir *= 1/length;

			// Get the orthogonal direction (in the xy plane)
			Vector3 xyDir(targetPosition.y() - _worldPosition.y(), _worldPosition.x() - targetPosition.x(), 0);
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
			_pointVector.push_back(VertexCb(_worldPosition));
			_pointVector.push_back(VertexCb(targetPosition));

			// The "arrow indicators" in the xy plane
			_pointVector.push_back(VertexCb(mid));
			_pointVector.push_back(VertexCb(xyPoint1));

			_pointVector.push_back(VertexCb(mid));
			_pointVector.push_back(VertexCb(xyPoint2));
		}
	}
};

/**
 * greebo: This is a helper object owned by the TargetableInstance.
 *         It represents a RenderablePointVector which repopulates
 *         itself each frame with the coordinates of the targeted
 *         instances. It provides a render() method.
 *
 * The render() method is invoked by the TargetableInstance during the
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

	void render(RenderableCollector& collector, const VolumeTest& volume, const Vector3& worldPosition) {
		if (_targetKeys.empty()) {
			return;
		}

		// Clear the vector
		clear();

		// Populate the RenderablePointVector with all target coordinates
		TargetLinesPopulator populator(*this, worldPosition, volume);
		_targetKeys.forEachTarget(populator);

		// If we hold any objects now, add us as renderable
		if (!empty()) {
			collector.addRenderable(*this, Matrix4::getIdentity());
		}
	}
};

} // namespace entity

#endif /* _ENTITY_RENDERABLE_TARGETLINES_H_ */
