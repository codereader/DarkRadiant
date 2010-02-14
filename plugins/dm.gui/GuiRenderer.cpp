#include "GuiRenderer.h"

#include "igl.h"

namespace gui
{

GuiRenderer::GuiRenderer()
{}

void GuiRenderer::render()
{
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

}
