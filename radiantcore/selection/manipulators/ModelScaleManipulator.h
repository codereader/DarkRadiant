#pragma once

#include <list>
#include "ManipulatorBase.h"
#include "ManipulatorComponents.h"

#include "selection/Renderables.h"
#include "selection/ManipulationPivot.h"
#include "selection/Pivot2World.h"
#include "selection/BasicSelectable.h"

#include "entitylib.h"

class Entity;

namespace selection
{

class ModelScaleManipulator :
	public ManipulatorBase
{
private:
	ManipulationPivot& _pivot;

	// Resize component
	ModelScaleComponent _scaleComponent;

	std::list<RenderableSolidAABB> _renderableAabbs;
	RenderablePointVector _renderableCornerPoints;

	scene::INodePtr _curManipulatable;
	
public:
	static ShaderPtr _lineShader;
	static ShaderPtr _pointShader;

	ModelScaleManipulator(ManipulationPivot& pivot);

	Type getType() const override;
	Component* getActiveComponent() override;
	void testSelect(const render::View& view, const Matrix4& pivot2world) override;
	void setSelected(bool select) override;
	bool isSelected() const override;
	void render(RenderableCollector& collector, const VolumeTest& volume) override;

private:
	void foreachSelectedTransformable(
		const std::function<void(const scene::INodePtr&, Entity*)>& functor);
};

}
