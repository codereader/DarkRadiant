#include "ModelScaleManipulator.h"

#include "selection/Remap.h"
#include "selection/BestPoint.h"
#include "selection/BestSelector.h"
#include "selection/SelectionTest.h"
#include "render/View.h"
#include "string/convert.h"
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
	_pivot2World.update(_pivot.getMatrix4(), view.GetModelview(), view.GetProjection(), view.GetViewport());

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
				rMessage() << "Got the point " << points[i] << std::endl;
				_curManipulatable = node;

				// We use the opposite corner as scale pivot
				Vector3 scalePivot = aabb.origin * 2 - points[i];

				_scaleComponent.setEntityNode(node);
				_scaleComponent.setScalePivot(scalePivot);

				rMessage() << "Got a node to manipulate: " << _curManipulatable << std::endl;

				break;
			}
#if 0
			SelectionIntersection intersection;
			volume.TestPoint(points[i], intersection);

			if (intersection.isValid() && intersection < best)
			{
				best = intersection;
				candidate = node;
			}
#endif
		}
	});

#if 0
	{
		Matrix4 local2view(view.GetViewMatrix().getMultipliedBy(_pivot2World._worldSpace));

		{
			SelectionIntersection best;
			Line_BestPoint(local2view, &_arrowX.front(), best);
			selector.addSelectable(best, &_selectableX);
		}

		{
			SelectionIntersection best;
			Line_BestPoint(local2view, &_arrowY.front(), best);
			selector.addSelectable(best, &_selectableY);
		}

		{
			SelectionIntersection best;
			Line_BestPoint(local2view, &_arrowZ.front(), best);
			selector.addSelectable(best, &_selectableZ);
		}
	}

	{
		Matrix4 local2view(view.GetViewMatrix().getMultipliedBy(_pivot2World._viewpointSpace));

		{
			SelectionIntersection best;
			Quad_BestPoint(local2view, eClipCullCW, &_quadScreen.front(), best);
			selector.addSelectable(best, &_selectableScreen);
		}
	}

	if (!selector.empty())
	{
		(*selector.begin()).second->setSelected(true);
	}
#endif
}

void ModelScaleManipulator::setSelected(bool select)
{
	_curManipulatable.reset();
}

bool ModelScaleManipulator::isSelected() const
{
	return _curManipulatable != nullptr;
}

void ModelScaleManipulator::render(RenderableCollector& collector, const VolumeTest& volume)
{
	_pivot2World.update(_pivot.getMatrix4(), volume.GetModelview(), volume.GetProjection(), volume.GetViewport());

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

	collector.SetState(_lineShader, RenderableCollector::eWireframeOnly);
	collector.SetState(_lineShader, RenderableCollector::eFullMaterials);

	for (const RenderableSolidAABB& aabb : _renderableAabbs)
	{
		collector.addRenderable(aabb, Matrix4::getIdentity());
	}

	collector.SetState(_pointShader, RenderableCollector::eWireframeOnly);
	collector.SetState(_pointShader, RenderableCollector::eFullMaterials);

	collector.addRenderable(_renderableCornerPoints, Matrix4::getIdentity());

	//collector.addRenderable(_arrowX, _pivot2World._worldSpace);
	//collector.addRenderable(_arrowY, _pivot2World._worldSpace);
	//collector.addRenderable(_arrowZ, _pivot2World._worldSpace);

	//collector.addRenderable(_quadScreen, _pivot2World._viewpointSpace);
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

