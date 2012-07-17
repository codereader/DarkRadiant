#include "GuiRenderer.h"

#include "igl.h"
#include "ifonts.h"
#include "ishaders.h"
#include "math/Matrix4.h"

namespace gui
{

GuiRenderer::GuiRenderer() :
	_areaTopLeft(0,0),
	_areaBottomRight(640, 480),
	_ignoreVisibility(false)
{}

void GuiRenderer::setGui(const GuiPtr& gui)
{
	_gui = gui;
}

void GuiRenderer::setVisibleArea(const Vector2& topLeft, const Vector2& bottomRight)
{
	_areaTopLeft = topLeft;
	_areaBottomRight = bottomRight;
}

void GuiRenderer::setIgnoreVisibility(bool ignoreVisibility)
{
	_ignoreVisibility = ignoreVisibility;
}

void GuiRenderer::render()
{
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// set up viewpoint
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Initialise the 2D projection matrix with: left, right, bottom, top, znear, zfar
	glOrtho(_areaTopLeft[0], 	// left
		_areaBottomRight[0], // right
		_areaBottomRight[1], // bottom
		_areaTopLeft[1], 	// top
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

	if (!window->visible && !_ignoreVisibility) return;

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
	if (window->backgroundShader != NULL && (window->matcolor[3] > 0 || _ignoreVisibility))
	{
		// Get the diffuse layer
		const ShaderLayerVector& layers = window->backgroundShader->getAllLayers();

		TexturePtr tex;

		for (ShaderLayerVector::const_iterator i = layers.begin(); i != layers.end(); ++i)
		{
			if ((*i)->getType() == ShaderLayer::DIFFUSE)
			{
				// Found the diffuse
				tex = (*i)->getTexture();
				break;
			}
		}

		if (tex == NULL)
		{
			tex = window->backgroundShader->getEditorImage();
		}

		if (tex != NULL)
		{
			glBindTexture(GL_TEXTURE_2D, tex->getGLTexNum());

			// Draw the textured quad
			glColor4dv(window->matcolor);

			// Render background image as opaque if _ignoreVisibility is set to true
			if (_ignoreVisibility && window->matcolor[3] <= 0)
			{
				glColor4d(window->matcolor[0], window->matcolor[1], window->matcolor[2], 1);
			}

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
	}

	// Render the text
	if (!window->getText().empty())
	{
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glEnable(GL_TEXTURE_2D);
		glColor4dv(window->forecolor);

		// Render text as opaque if _ignoreVisibility is set to true
		if (_ignoreVisibility && window->forecolor[3] <= 0)
		{
			glColor4d(window->forecolor[0], window->forecolor[1], window->forecolor[2], 1);
		}

		window->getRenderableText().render();

		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	// Push the translation before rendering the children, so that they
	// can continue rendering in local coordinates, but the results appear relative to
	// this parent windowDef
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslated(window->rect[0], window->rect[1], 0);

	for (GuiWindowDef::ChildWindows::const_iterator i = window->children.begin();
		 i != window->children.end(); ++i)
	{
		render(*i);
	}

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

} // namespace
