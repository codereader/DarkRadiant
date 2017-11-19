#pragma once

#include "igui.h"
#include "math/Vector2.h"
#include "util/Noncopyable.h"

#include <vector>

namespace wxutil
{

class GuiRenderer :
	public util::Noncopyable
{
private:
	gui::IGuiPtr _gui;

	Vector2 _areaTopLeft;
	Vector2 _areaBottomRight;

	// Whether invisible windowDefs should be rendered anyway
	bool _ignoreVisibility;

	std::string _windowDefFilter;

public:
	// Construct a new renderer
	GuiRenderer();

	void setGui(const gui::IGuiPtr& gui);

	void setIgnoreVisibility(bool ignoreVisibility);

	// Sets the visible area to be rendered, in absolute GUI coordinates [0,0..640,480]
	void setVisibleArea(const Vector2& topLeft, const Vector2& bottomRight);

	// Only draw this windowDef and its children.
	void setWindowDefFilter(const std::string& windowDef);

	// Starts rendering the attached GUI
	// The GL context must be set before calling this method
	void render();

private:
	void render(const gui::IGuiWindowDefPtr& window, bool ignoreFilter = false);
};

}
