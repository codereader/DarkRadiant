#include "GuiRenderer.h"

#include "igl.h"
#include "ishaders.h"

namespace gui
{

GuiRenderer::GuiRenderer() :
	_viewPortTopLeft(0,0),
	_viewPortBottomRight(640, 480)
{}

void GuiRenderer::setGui(const GuiPtr& gui)
{
	_gui = gui;
}

void GuiRenderer::render()
{
	glViewport(
		static_cast<GLsizei>(_viewPortTopLeft[0]), 
		static_cast<GLsizei>(_viewPortTopLeft[1]), 
		static_cast<GLsizei>(_viewPortBottomRight[0]), 
		static_cast<GLsizei>(_viewPortBottomRight[1])
	);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Fetch the desktop windowDef and render it
	render(_gui->getDesktop());
}

void GuiRenderer::render(const GuiWindowDefPtr& window)
{
	// set up viewpoint
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Initialise the 2D projection matrix with: left, right, bottom, top, znear, zfar 
	glOrtho(_viewPortTopLeft[0], 	// left 
		_viewPortBottomRight[0], // right
		_viewPortBottomRight[1], // bottom 
		_viewPortTopLeft[1], 	// top 
		-1, 1);

	// Tell openGL to draw front and back of the polygons in textured mode
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glColor3f(1, 1, 1);

	// Acquire the texture number of the active texture
	MaterialPtr shader = GlobalMaterialManager().getMaterialForName("textures/darkmod/stone/flat/ceramic_mosaic_decorative01");
	TexturePtr tex = shader->getEditorImage();
	glBindTexture(GL_TEXTURE_2D, tex->getGLTexNum());

	// Draw the background texture
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);

	glTexCoord2f(0,0);
	glVertex2f(0,0);	// Upper left

	glTexCoord2f(1,0);
	glVertex2f(640,0);	// Upper right

	glTexCoord2f(1,1);
	glVertex2f(640,480);	// Lower right

	glTexCoord2f(0,1);
	glVertex2f(0, 480);	// Lower left

	glEnd();
	glDisable(GL_TEXTURE_2D);
}

} // namespace
