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
	_scaleFree(*this),
	_renderableCornerPoints(GL_POINTS)
{
	draw_arrowline(64, &_arrowX.front(), 0);
	draw_arrowline(64, &_arrowY.front(), 1);
	draw_arrowline(64, &_arrowZ.front(), 2);

	draw_quad(16, &_quadScreen.front());
}

ModelScaleManipulator::Type ModelScaleManipulator::getType() const
{
	return Type::ModelScale;
}

ModelScaleManipulator::Component* ModelScaleManipulator::getActiveComponent()
{
	return &_scaleFree;
}

void ModelScaleManipulator::testSelect(const render::View& view, const Matrix4& pivot2world)
{
	_pivot2World.update(_pivot.getMatrix4(), view.GetModelview(), view.GetProjection(), view.GetViewport());

	_curManipulatable.reset();

	BestSelector selector;
	SelectionVolume volume(view);
	SelectionIntersection best;
	scene::INodePtr candidate;
	
	foreachSelectedTransformable([&](const scene::INodePtr& node, Entity* entity)
	{
		const AABB& aabb = node->worldAABB();
		
		Vector3 points[8];
		aabb.getCorners(points);

		for (std::size_t i = 0; i < 8; ++i)
		{
			if (view.TestPoint(points[i]))
			{
				candidate = node;
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

	if (candidate)
	{
		_curManipulatable = candidate;
		rMessage() << "Got a node to manipulate: " << _curManipulatable << std::endl;
	}

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

void ModelScaleManipulator::scale(const Vector3& scaling)
{
	rMessage() << "Scale: " << scaling << std::endl;

	if (_curManipulatable)
	{
		Entity* entity = Node_getEntity(_curManipulatable);

		// Apply the scaling spawnarg to this entity
		entity->setKeyValue("dr_model_scale", string::to_string(scaling));
	}

	// Update the scene views
	SceneChangeNotify();
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

