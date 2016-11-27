#pragma once

#include "ManipulatorBase.h"
#include "../Renderables.h"
#include "../Pivot2World.h"
#include "ManipulatorComponents.h"
#include "../BasicSelectable.h"

namespace selection
{

/**
 * Manipulator for performing axis-aligned translations.
 */
class TranslateManipulator : 
	public ManipulatorBase
{
private:
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
	Pivot2World _pivot;
public:
	static ShaderPtr _stateWire;
	static ShaderPtr _stateFill;

	// Constructor
	TranslateManipulator(Translatable& translatable, std::size_t segments, float length);

	Type getType() const override
	{
		return Translate;
	}

	void UpdateColours();
	bool manipulator_show_axis(const Pivot2World& pivot, const Vector3& axis);

	void render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& pivot2world) override;
	void testSelect(const render::View& view, const Matrix4& pivot2world) override;
	Component* getActiveComponent() override;

	void setSelected(bool select) override;
	bool isSelected() const override;
};

}
