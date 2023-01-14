#include "GuiRenderer.h"

#include "igl.h"
#include "ifonts.h"
#include "ishaders.h"
#include "math/Matrix4.h"

namespace wxutil
{

GuiRenderer::GuiRenderer() :
	_areaTopLeft(0,0),
	_areaBottomRight(640, 480),
	_ignoreVisibility(false)
{}

void GuiRenderer::setGui(const gui::IGuiPtr& gui)
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

void GuiRenderer::setWindowDefFilter(const std::string& windowDef)
{
	_windowDefFilter = windowDef;
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

void GuiRenderer::render(const gui::IGuiWindowDefPtr& window, bool ignoreFilter)
{
	if (window == NULL) return;

	if (!window->visible && !_ignoreVisibility) return;

	if (!ignoreFilter && !_windowDefFilter.empty())
	{
		if (window->name == _windowDefFilter)
		{
			ignoreFilter = true; // this is the window we want, ignore filter from here on
		}
		else if (!window->findWindowDef(_windowDefFilter))
		{
			return; // filtered window is not even a child
		}
	}

	Vector4 rect = window->rect;
	Vector4 backcolor = window->backcolor;

	if (backcolor[3] > 0)
	{
		glColor4dv(backcolor);

		// Background quad
		glBegin(GL_QUADS);
		glVertex2d(rect[0], rect[1]);	// Upper left
		glVertex2d(rect[0] + rect[2], rect[1]); // Upper right
		glVertex2d(rect[0] + rect[2], rect[1] + rect[3]); // Lower right
		glVertex2d(rect[0], rect[1] + rect[3]); // Lower left
		glEnd();
	}

	// Realise background shader if necessary
	if (!window->background.getValue().empty() && window->backgroundShader == NULL)
	{
		window->backgroundShader = GlobalMaterialManager().getMaterial(window->background);
	}

	// Acquire the texture number of the active texture
	Vector4 matcolor = window->matcolor;

	if (window->backgroundShader != NULL && (matcolor[3] > 0 || _ignoreVisibility))
	{
		// Get the diffuse layer
		TexturePtr tex;

        window->backgroundShader->foreachLayer([&](const IShaderLayer::Ptr& layer)
		{
			if (layer->getType() == IShaderLayer::DIFFUSE)
			{
				// Found the diffuse
				tex = layer->getTexture();
				return false;
			}
            return true;
		});

		if (tex == NULL)
		{
			tex = window->backgroundShader->getEditorImage();
		}

		if (tex != NULL)
		{
			glBindTexture(GL_TEXTURE_2D, tex->getGLTexNum());

			// Draw the textured quad
			glColor4dv(matcolor);

			// Render background image as opaque if _ignoreVisibility is set to true
			if (_ignoreVisibility && matcolor[3] <= 0)
			{
				glColor4d(matcolor[0], matcolor[1], matcolor[2], 1);
			}

			glEnable(GL_TEXTURE_2D);
			glBegin(GL_QUADS);

			glTexCoord2f(0,0);
			glVertex2d(rect[0], rect[1]);	// Upper left

			glTexCoord2f(1,0);
			glVertex2d(rect[0] + rect[2], rect[1]); // Upper right

			glTexCoord2f(1,1);
			glVertex2d(rect[0] + rect[2], rect[1] + rect[3]); // Lower right

			glTexCoord2f(0,1);
			glVertex2d(rect[0], rect[1] + rect[3]); // Lower left

			glEnd();
			glDisable(GL_TEXTURE_2D);
		}
	}

	// Render the text
	if (!window->text.getValue().empty())
	{
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glEnable(GL_TEXTURE_2D);
		Vector4 forecolor = window->forecolor;
		glColor4dv(forecolor);

		// Render text as opaque if _ignoreVisibility is set to true
		if (_ignoreVisibility && forecolor[3] <= 0)
		{
			glColor4d(forecolor[0], forecolor[1], forecolor[2], 1);
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
	glTranslated(rect[0], rect[1], 0);

	for (const gui::IGuiWindowDefPtr& child : window->children)
	{
		render(child, ignoreFilter);
	}

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

} // namespace
