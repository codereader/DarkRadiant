#include "ModelScaleManipulator.h"

#include "render/View.h"
#include "debugging/ScenegraphUtils.h"

namespace selection
{

ModelScaleManipulator::ModelScaleManipulator(ManipulationPivot& pivot) :
	_pivot(pivot),
	_renderableCornerPoints(GL_POINTS)
{
}

ModelScaleManipulator::Type ModelScaleManipulator::getType() const
{
	return Type::ModelScale;
}

ModelScaleManipulator::Component* ModelScaleManipulator::getActiveComponent()
{
	return &_scaleComponent;
}

void ModelScaleManipulator::testSelect(const render::View& view, const Matrix4& pivot2world)
{
	_curManipulatable.reset();
	_scaleComponent.setEntityNode(scene::INodePtr());
	_scaleComponent.setScalePivot(Vector3(0, 0, 0));

	foreachSelectedTransformable([&](const scene::INodePtr& node, Entity* entity)
	{
		if (_curManipulatable) return; // already done here

		const AABB& aabb = node->worldAABB();
		
		Vector3 points[8];
		aabb.getCorners(points);

		for (std::size_t i = 0; i < 8; ++i)
		{
			if (view.TestPoint(points[i]))
			{
				_curManipulatable = node;

				// We use the opposite corner as scale pivot
				Vector3 scalePivot = aabb.origin * 2 - points[i];

				_scaleComponent.setEntityNode(node);
				_scaleComponent.setScalePivot(scalePivot);

				break;
			}
		}
	});
}

void ModelScaleManipulator::setSelected(bool select)
{
	_curManipulatable.reset();
	_scaleComponent.setEntityNode(scene::INodePtr());
}

bool ModelScaleManipulator::isSelected() const
{
	return _curManipulatable != nullptr;
}

void ModelScaleManipulator::render(RenderableCollector& collector, const VolumeTest& volume)
{
	_renderableAabbs.clear();
	_renderableCornerPoints.clear();
	
	foreachSelectedTransformable([&](const scene::INodePtr& node, Entity* entity)
	{
		const AABB& aabb = node->worldAABB();
		_renderableAabbs.push_back(RenderableSolidAABB(aabb));

		Vector3 points[8];
		aabb.getCorners(points);

		bool isSelected = (node == _curManipulatable);

		for (std::size_t i = 0; i < 8; ++i)
		{
			_renderableCornerPoints.push_back(VertexCb(points[i], isSelected ? COLOUR_SELECTED() : COLOUR_SCREEN()));
		}
	});

	for (const RenderableSolidAABB& aabb : _renderableAabbs)
	{
		collector.addRenderable(_lineShader, aabb, Matrix4::getIdentity());
	}

	collector.addRenderable(_pointShader, _renderableCornerPoints, Matrix4::getIdentity());
}

void ModelScaleManipulator::foreachSelectedTransformable(
	const std::function<void(const scene::INodePtr&, Entity*)>& functor)
{
	GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
	{
		Entity* entity = Node_getEntity(node);

		if (entity && entity->isModel())
		{
			functor(node, entity);
		}
	});
}

ShaderPtr ModelScaleManipulator::_lineShader;
ShaderPtr ModelScaleManipulator::_pointShader;

}

