#ifndef _ENTITY_RENDERABLE_TARGETLINES_H_
#define _ENTITY_RENDERABLE_TARGETLINES_H_

#include "TargetKeyCollection.h"
#include "render.h"
#include "irenderable.h"
#include "cullable.h"
#include "math/line.h"

namespace entity {

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

	virtual void visit(const entity::TargetPtr& target) {
		if (target->isEmpty()) {
			return;
		}

		Vector3 targetPosition = target->getPosition();
		if (_volume.TestLine(segment_for_startend(_worldPosition, targetPosition))) {
			_pointVector.push_back(PointVertex(reinterpret_cast<const Vertex3f&>(_worldPosition)));
			_pointVector.push_back(PointVertex(reinterpret_cast<const Vertex3f&>(targetPosition)));
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

	void render(Renderer& renderer, const VolumeTest& volume, const Vector3& worldPosition) {
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
			renderer.addRenderable(*this, g_matrix4_identity);
		}
	}
};

} // namespace entity

#endif /* _ENTITY_RENDERABLE_TARGETLINES_H_ */
