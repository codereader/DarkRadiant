#include "ModelScaleManipulator.h"

#include "debugging/ScenegraphUtils.h"

namespace selection
{

ModelScaleManipulator::ModelScaleManipulator(ManipulationPivot& pivot) :
	_pivot(pivot),
    _renderableAABBs(_aabbs),
    _renderableCornerPoints(_aabbs)
{
}

ModelScaleManipulator::~ModelScaleManipulator()
{
    clearRenderables();
}

ModelScaleManipulator::Type ModelScaleManipulator::getType() const
{
	return Type::ModelScale;
}

ModelScaleManipulator::Component* ModelScaleManipulator::getActiveComponent()
{
	return &_scaleComponent;
}

void ModelScaleManipulator::testSelect(SelectionTest& test, const Matrix4& pivot2world)
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
			if (test.getVolume().TestPoint(points[i]))
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

void ModelScaleManipulator::onPreRender(const RenderSystemPtr& renderSystem, const VolumeTest& volume)
{
    if (!renderSystem)
    {
        clearRenderables();
        _aabbs.clear();
        return;
    }

    if (!_lineShader)
    {
        _lineShader = renderSystem->capture(BuiltInShaderType::WireframeOverlay);
    }

    if (!_pointShader)
    {
        _pointShader = renderSystem->capture(BuiltInShaderType::BigPoint);
    }
    
    _aabbs.clear();

    foreachSelectedTransformable([&](const scene::INodePtr& node, Entity* entity)
    {
        _aabbs.push_back(node->worldAABB());
    });

    _renderableCornerPoints.setColour(isSelected() ? COLOUR_SELECTED() : COLOUR_SCREEN());
    _renderableCornerPoints.queueUpdate();
    _renderableAABBs.queueUpdate();

    _renderableAABBs.update(_lineShader);
    _renderableCornerPoints.update(_pointShader);
}

void ModelScaleManipulator::render(IRenderableCollector& collector, const VolumeTest& volume)
{
}

void ModelScaleManipulator::clearRenderables()
{
    _renderableCornerPoints.clear();
    _renderableAABBs.clear();
    _lineShader.reset();
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

}

