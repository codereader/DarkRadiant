#include "ModelScaleManipulator.h"

#include "selection/Remap.h"
#include "selection/BestPoint.h"
#include "selection/SelectionPool.h"
#include "render/View.h"
#include "string/convert.h"

namespace selection
{

ModelScaleManipulator::ModelScaleManipulator(ManipulationPivot& pivot) :
	_pivot(pivot),
	_scaleFree(*this)
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

	SelectionPool selector;

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
#endif

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
}

void ModelScaleManipulator::setSelected(bool select)
{
	_selectableScreen.setSelected(select);
}

bool ModelScaleManipulator::isSelected() const
{
	return _selectableScreen.isSelected();
}

void ModelScaleManipulator::render(RenderableCollector& collector, const VolumeTest& volume)
{
	_pivot2World.update(_pivot.getMatrix4(), volume.GetModelview(), volume.GetProjection(), volume.GetViewport());

	//collector.addRenderable(_arrowX, _pivot2World._worldSpace);
	//collector.addRenderable(_arrowY, _pivot2World._worldSpace);
	//collector.addRenderable(_arrowZ, _pivot2World._worldSpace);

	collector.addRenderable(_quadScreen, _pivot2World._viewpointSpace);
}

void ModelScaleManipulator::scale(const Vector3& scaling)
{
	rMessage() << "Scale: " << scaling << std::endl;

	GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
	{
		Entity* entity = Node_getEntity(node);

		if (entity && entity->isModel())
		{
			// Apply the scaling spawnarg to this entity
			entity->setKeyValue("dr_model_scale", string::to_string(scaling));
		}
	});

	// Update the scene views
	SceneChangeNotify();
}

}

