#pragma once

#include "ManipulatorBase.h"
#include "selection/Renderables.h"
#include "selection/Pivot2World.h"
#include "ManipulatorComponents.h"
#include "selection/BasicSelectable.h"
#include "selection/ManipulationPivot.h"
#include "Renderables.h"

namespace selection
{

/**
 * Manipulator for performing axis-aligned translations.
 */
class TranslateManipulator : 
	public ManipulatorBase
{
private:
	ManipulationPivot& _pivot;

	SelectionTranslator _translator;
	TranslateFree _translateFree;
	TranslateAxis _translateAxis;
	RenderableArrowLine _arrowX;
	RenderableArrowLine _arrowY;
	RenderableArrowLine _arrowZ;
	RenderableArrowHead _arrowHeadX;
	RenderableArrowHead _arrowHeadY;
	RenderableArrowHead _arrowHeadZ;
	RenderableQuad _quadScreen;
	selection::BasicSelectable _selectableX;
	selection::BasicSelectable _selectableY;
	selection::BasicSelectable _selectableZ;
	selection::BasicSelectable _selectableScreen;
	Pivot2World _pivot2World;

    ShaderPtr _lineShader;

public:
	static ShaderPtr _stateWire;
	static ShaderPtr _stateFill;

	// Constructor
	TranslateManipulator(ManipulationPivot& pivot, std::size_t segments, float length);

	Type getType() const override
	{
		return Translate;
	}

    void onPreRender(const RenderSystemPtr& renderSystem, const VolumeTest& volume) override;
	void render(IRenderableCollector& collector, const VolumeTest& volume) override;
    void clearRenderables() override;

	void testSelect(SelectionTest& test, const Matrix4& pivot2world) override;
	Component* getActiveComponent() override;

	void setSelected(bool select) override;
	bool isSelected() const override;

private:
    void updateColours();
    bool axisIsVisible(const Vector3& axis) const;
};

}
