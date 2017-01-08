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
	public ManipulatorBase,
	public Scalable
{
private:
	ManipulationPivot& _pivot;

	// Resize component
	ScaleFree _scaleFree;

	RenderableArrowLine _arrowX;
	RenderableArrowLine _arrowY;
	RenderableArrowLine _arrowZ;
	RenderableQuad _quadScreen;

	selection::BasicSelectable _selectableScreen;

	Pivot2World _pivot2World;

	std::list<RenderableSolidAABB> _renderableAabbs;
	
public:
	static ShaderPtr _lineShader;

	ModelScaleManipulator(ManipulationPivot& pivot);

	Type getType() const override;
	Component* getActiveComponent() override;
	void testSelect(const render::View& view, const Matrix4& pivot2world) override;
	void setSelected(bool select) override;
	bool isSelected() const override;
	void render(RenderableCollector& collector, const VolumeTest& volume) override;

	void scale(const Vector3& scaling) override;

private:
	void foreachSelectedTransformable(
		const std::function<void(const scene::INodePtr&, Entity*)>& functor);
};

}
