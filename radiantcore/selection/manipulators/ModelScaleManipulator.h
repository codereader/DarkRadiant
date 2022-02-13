#pragma once

#include <list>
#include "ManipulatorBase.h"
#include "ManipulatorComponents.h"

#include "selection/ManipulationPivot.h"
#include "selection/Pivot2World.h"
#include "selection/BasicSelectable.h"

#include "entitylib.h"
#include "Renderables.h"
#include "render/RenderableBoundingBoxes.h"

class Entity;

namespace selection
{

class ModelScaleManipulator final :
	public ManipulatorBase
{
private:
	ManipulationPivot& _pivot;

	// Resize component
	ModelScaleComponent _scaleComponent;

    ShaderPtr _pointShader;
    ShaderPtr _lineShader;

	std::vector<AABB> _aabbs;
    render::RenderableBoundingBoxes _renderableAABBs;
    RenderableCornerPoints _renderableCornerPoints;

	scene::INodePtr _curManipulatable;
	
public:
	ModelScaleManipulator(ManipulationPivot& pivot);
    ~ModelScaleManipulator();

	Type getType() const override;
	Component* getActiveComponent() override;
	void testSelect(SelectionTest& test, const Matrix4& pivot2world) override;
	void setSelected(bool select) override;
	bool isSelected() const override;
    void onPreRender(const RenderSystemPtr& renderSystem, const VolumeTest& volume) override;
	void render(IRenderableCollector& collector, const VolumeTest& volume) override;
    void clearRenderables() override;

private:
	void foreachSelectedTransformable(
		const std::function<void(const scene::INodePtr&, Entity*)>& functor);
};

}
