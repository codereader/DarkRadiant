#include "ReadableGuiView.h"

namespace gui
{

void ReadableGuiView::setGLViewPort()
{
	double width = _windowDims[0];
	double height = _windowDims[1];

	double aspectRatio = _bgDims[0] / _bgDims[1];

	if (width / height > aspectRatio)
	{
		width = height * aspectRatio;
	}
	else
	{
		height = width / aspectRatio;
	}

	set_size_request(static_cast<int>(width), -1);

	glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

void ReadableGuiView::setGui(const GuiPtr& gui)
{
	// Call the base class first
	GuiView::setGui(gui);

	Vector2 topLeft(0, 0);
	Vector2 bottomRight(640, 480);

	if (_gui != NULL)
	{
		GuiWindowDefPtr bgWindowDef = _gui->findWindowDef("backgroundImage");
		if (!bgWindowDef)
		{
			bgWindowDef = _gui->findWindowDef("backgroundmulti");
			if (!bgWindowDef)
			{
				bgWindowDef = _gui->findWindowDef("backgroundsingle");
			}
		}

		if (bgWindowDef != NULL)
		{
			const Vector4& rect = bgWindowDef->rect;
			topLeft = Vector2(rect[0], rect[1]);
			bottomRight = Vector2(rect[0] + rect[2], rect[1] + rect[3]);
		}
	}

	_bgDims = bottomRight - topLeft;

	_renderer.setVisibleArea(topLeft, bottomRight);
}

} // namespace
