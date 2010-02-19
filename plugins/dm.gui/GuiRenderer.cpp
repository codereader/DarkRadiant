#include "GuiRenderer.h"

#include "igl.h"
#include "ifonts.h"
#include "ishaders.h"
#include "math/matrix.h"

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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (_gui != NULL)
	{
		// Fetch the desktop windowDef and render it
		render(_gui->getDesktop());
	}

	glDisable(GL_BLEND);
}

void GuiRenderer::render(const GuiWindowDefPtr& window)
{
	if (window == NULL) return;

	if (!window->visible) return;

	if (window->backcolor[3] > 0)
	{
		glColor4dv(window->backcolor);

		// Background quad
		glBegin(GL_QUADS);
		glVertex2d(window->rect[0], window->rect[1]);	// Upper left
		glVertex2d(window->rect[0] + window->rect[2], window->rect[1]); // Upper right
		glVertex2d(window->rect[0] + window->rect[2], window->rect[1] + window->rect[3]); // Lower right
		glVertex2d(window->rect[0], window->rect[1] + window->rect[3]); // Lower left
		glEnd();
	}

	// Realise background shader if necessary
	if (!window->background.empty() && window->backgroundShader == NULL)
	{
		window->backgroundShader = GlobalMaterialManager().getMaterialForName(window->background);
	}

	// Acquire the texture number of the active texture
	if (window->matcolor[3] > 0 && window->backgroundShader != NULL)
	{
		TexturePtr tex = window->backgroundShader->getEditorImage();
		glBindTexture(GL_TEXTURE_2D, tex->getGLTexNum());

		// Draw the textured quad
		glColor4dv(window->matcolor);
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);

		glTexCoord2f(0,0);
		glVertex2d(window->rect[0], window->rect[1]);	// Upper left

		glTexCoord2f(1,0);
		glVertex2d(window->rect[0] + window->rect[2], window->rect[1]); // Upper right

		glTexCoord2f(1,1);
		glVertex2d(window->rect[0] + window->rect[2], window->rect[1] + window->rect[3]); // Lower right

		glTexCoord2f(0,1);
		glVertex2d(window->rect[0], window->rect[1] + window->rect[3]); // Lower left

		glEnd();
		glDisable(GL_TEXTURE_2D);
	}

	// Render the text
	if (!window->getText().empty())
	{
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glEnable(GL_TEXTURE_2D);
		glColor4dv(window->forecolor);
		
		window->getRenderableText().render();

		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	for (GuiWindowDef::ChildWindows::const_iterator i = window->children.begin();
		 i != window->children.end(); ++i)
	{
		render(*i);
	}
}

} // namespace
