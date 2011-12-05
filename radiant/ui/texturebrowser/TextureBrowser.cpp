#include "TextureBrowser.h"

#include "i18n.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "iradiant.h"
#include "ipreferencesystem.h"

#include "gtkutil/GLWidgetSentry.h"
#include "gtkutil/IconTextMenuItem.h"
#include "gtkutil/LeftAlignedLabel.h"

#include "registry/registry.h"
#include "shaderlib.h"
#include "selection/algorithm/Shader.h"
#include "ui/mediabrowser/MediaBrowser.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/bind.hpp>

#include <gtkmm/box.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/entry.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/toolbar.h>

namespace ui
{

namespace
{
	const std::string RKEY_TEXTURES_HIDE_UNUSED = "user/ui/textures/browser/hideUnused";
	const std::string RKEY_TEXTURE_SCALE = "user/ui/textures/browser/textureScale";
	const std::string RKEY_TEXTURE_UNIFORM_SIZE = "user/ui/textures/browser/uniformSize";
	const std::string RKEY_TEXTURE_SHOW_SCROLLBAR = "user/ui/textures/browser/showScrollBar";
	const std::string RKEY_TEXTURE_MOUSE_WHEEL_INCR = "user/ui/textures/browser/mouseWheelIncrement";
	const std::string RKEY_TEXTURE_SHOW_FILTER = "user/ui/textures/browser/showFilter";
	const std::string RKEY_TEXTURE_CONTEXTMENU_EPSILON = "user/ui/textures/browser/contextMenuMouseEpsilon";
	const std::string RKEY_TEXTURE_MAX_NAME_LENGTH = "user/ui/textures/browser/maxShadernameLength";

	const char* const SEEK_IN_MEDIA_BROWSER_TEXT = N_("Seek in Media Browser");
	const char* TEXTURE_ICON = "icon_texture.png";

	inline void doNothing() {}

	inline bool alwaysFalse() { return false; }
}

TextureBrowser::TextureBrowser() :
	_popupX(-1),
	_popupY(-1),
	_startOrigin(-1),
	_epsilon(registry::getValue<float>(RKEY_TEXTURE_CONTEXTMENU_EPSILON)),
	_popupMenu(new gtkutil::PopupMenu),
	_filter(NULL),
	_glWidget(NULL),
	_textureScrollbar(NULL),
	m_heightChanged(true),
	m_originInvalid(true),
	m_mouseWheelScrollIncrement(registry::getValue<int>(RKEY_TEXTURE_MOUSE_WHEEL_INCR)),
	m_textureScale(50),
	m_showTextureFilter(registry::getValue<bool>(RKEY_TEXTURE_SHOW_FILTER)),
	m_showTextureScrollbar(registry::getValue<bool>(RKEY_TEXTURE_SHOW_SCROLLBAR)),
	m_hideUnused(registry::getValue<bool>(RKEY_TEXTURES_HIDE_UNUSED)),
	m_resizeTextures(true),
	m_uniformTextureSize(registry::getValue<int>(RKEY_TEXTURE_UNIFORM_SIZE))
{
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURES_HIDE_UNUSED);
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURE_SCALE);
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURE_UNIFORM_SIZE);
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURE_SHOW_SCROLLBAR);
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURE_MOUSE_WHEEL_INCR);
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURE_SHOW_FILTER);

	_shader = texdef_name_default();

	setScaleFromRegistry();

	_shaderLabel = Gtk::manage(new Gtk::MenuItem(_("No shader")));
	_shaderLabel->set_sensitive(false); // always greyed out

	_popupMenu->addItem(_shaderLabel, doNothing, alwaysFalse); // always insensitive

	// Construct the popup context menu
	_seekInMediaBrowser = Gtk::manage(new gtkutil::IconTextMenuItem(
		GlobalUIManager().getLocalPixbuf(TEXTURE_ICON),
		_(SEEK_IN_MEDIA_BROWSER_TEXT)
	));

	_popupMenu->addItem(_seekInMediaBrowser,
						boost::bind(&TextureBrowser::onSeekInMediaBrowser, this),
						boost::bind(&TextureBrowser::checkSeekInMediaBrowser, this));
}

void TextureBrowser::queueDraw()
{
	if (_glWidget != NULL)
	{
		_glWidget->queueDraw();
	}
}

void TextureBrowser::textureModeChanged()
{
	queueDraw();
}

void TextureBrowser::setScaleFromRegistry()
{
	int index = registry::getValue<int>(RKEY_TEXTURE_SCALE);

	switch (index) {
		case 0: m_textureScale = 10; break;
		case 1: m_textureScale = 25; break;
		case 2: m_textureScale = 50; break;
		case 3: m_textureScale = 100; break;
		case 4: m_textureScale = 200; break;
	};

	queueDraw();
}

void TextureBrowser::clearFilter()
{
	_filter->set_text("");
	queueDraw();
}

void TextureBrowser::keyChanged(const std::string& key, const std::string& val)
{
	m_hideUnused = registry::getValue<bool>(RKEY_TEXTURES_HIDE_UNUSED);
	m_showTextureFilter = registry::getValue<bool>(RKEY_TEXTURE_SHOW_FILTER);
	m_uniformTextureSize = registry::getValue<int>(RKEY_TEXTURE_UNIFORM_SIZE);
	m_showTextureScrollbar = registry::getValue<bool>(RKEY_TEXTURE_SHOW_SCROLLBAR);
	m_mouseWheelScrollIncrement = registry::getValue<int>(RKEY_TEXTURE_MOUSE_WHEEL_INCR);

	if (m_showTextureScrollbar)
	{
		_textureScrollbar->show();
	}
	else
	{
		_textureScrollbar->hide();
	}

	if (m_showTextureFilter)
	{
		_filter->show();
	}
	else
	{
		_filter->hide();
	}

	setScaleFromRegistry();

	heightChanged();
	m_originInvalid = true;
}

// Return the display width of a texture in the texture browser
int TextureBrowser::getTextureWidth(const TexturePtr& tex)
{
    int width;

    if (!m_resizeTextures)
	{
		// Don't use uniform size
		width = static_cast<int>(tex->getWidth() * (static_cast<float>(m_textureScale) / 100));
	}
	else if (tex->getWidth() >= tex->getHeight())
	{
		// Texture is square, or wider than it is tall
		width = m_uniformTextureSize;
	}
	else
	{
		// Otherwise, preserve the texture's aspect ratio
		width = static_cast<int>(m_uniformTextureSize * (static_cast<float>(tex->getWidth()) / tex->getHeight()));
	}

    return width;
}

int TextureBrowser::getTextureHeight(const TexturePtr& tex)
{
	int height;

	if (!m_resizeTextures)
	{
		// Don't use uniform size
		height = static_cast<int>(tex->getHeight() * (static_cast<float>(m_textureScale) / 100));
	}
    else if (tex->getHeight() >= tex->getWidth())
	{
		// Texture is square, or taller than it is wide
		height = m_uniformTextureSize;
	}
	else
	{
		// Otherwise, preserve the texture's aspect ratio
		height = static_cast<int>(m_uniformTextureSize * (static_cast<float>(tex->getHeight()) / tex->getWidth()));
	}

	return height;
}

int TextureBrowser::getFontHeight()
{
	return GlobalOpenGL().getFontHeight();
}

const std::string& TextureBrowser::getSelectedShader() const
{
	return _shader;
}

std::string TextureBrowser::getFilter()
{
	return _filter->get_text();
}

void TextureBrowser::setSelectedShader(const std::string& newShader)
{
	_shader = newShader;
	focus(_shader);
}

void TextureBrowser::nextTexturePos(TextureLayout& layout, const TexturePtr& tex, int *x, int *y)
{
	int nWidth = getTextureWidth(tex);
	int nHeight = getTextureHeight(tex);

	// go to the next row unless the texture is the first on the row
	if (layout.current_x + nWidth > width-8 && layout.current_row)
	{
		layout.current_x = 8;
		layout.current_y -= layout.current_row + getFontHeight() + 4;
		layout.current_row = 0;
	}

	// Emit the new coordinates
	*x = layout.current_x;
	*y = layout.current_y;

	// Is our texture larger than the row? If so, grow the
	// row height to match it

	if (layout.current_row < nHeight)
	{
		layout.current_row = nHeight;
	}

	// never go less than 96, or the names get all crunched up, plus spacing
	layout.current_x += std::max(96, nWidth) + 8;
}

// if texture_showinuse jump over non in-use textures
bool TextureBrowser::shaderIsVisible(const MaterialPtr& shader)
{
	if (shader == NULL)
	{
		return false;
	}

	if (!boost::algorithm::istarts_with(shader->getName(), "textures/"))
    {
		return false;
	}

	if (m_hideUnused && !shader->IsInUse())
	{
		return false;
	}

  	if (!getFilter().empty())
    {
		// some basic filtering
		if (strstr( shader_get_textureName(shader->getName().c_str()), getFilter().c_str() ) == 0)
			return false;
	}

	return true;
}

void TextureBrowser::heightChanged()
{
	m_heightChanged = true;

	updateScroll();
	queueDraw();
}

void TextureBrowser::evaluateHeight()
{
	// greebo: Let the texture browser re-evaluate the scrollbar each frame
   m_heightChanged = false;
   m_nTotalHeight = 0;

   if (GlobalMaterialManager().isRealised())
   {
	   class HeightWalker :
			public shaders::ShaderVisitor
		{
		private:
			TextureBrowser& _browser;
			TextureLayout _layout;
			int _totalHeight;

		public:
			HeightWalker(TextureBrowser& browser) :
				_browser(browser),
				_totalHeight(0)
			{}

			void visit(const MaterialPtr& shader)
			{
				if (!_browser.shaderIsVisible(shader))
				{
					return;
				}

				int x, y;
				_browser.nextTexturePos(_layout, shader->getEditorImage(), &x, &y);
				_totalHeight = std::max(_totalHeight, abs(_layout.current_y) + _browser.getFontHeight() + _browser.getTextureHeight(shader->getEditorImage()) + 4);
			}

			int getTotalHeight()
			{
				return _totalHeight;
			}
		} _walker(*this);

		GlobalMaterialManager().foreachShader(_walker);

		m_nTotalHeight = _walker.getTotalHeight();
   }
}

int TextureBrowser::getTotalHeight()
{
	evaluateHeight();
	return m_nTotalHeight;
}

void TextureBrowser::clampOriginY()
{
	if (originy > 0)
	{
		originy = 0;
	}

	int lower = std::min(height - getTotalHeight(), 0);

	if (originy < lower)
	{
		originy = lower;
	}
}

int TextureBrowser::getOriginY()
{
	if (m_originInvalid)
	{
		m_originInvalid = false;
		clampOriginY();
		updateScroll();
	}

	return originy;
}

void TextureBrowser::setOriginY(int newOriginY)
{
	originy = newOriginY;
	clampOriginY();
	updateScroll();
	queueDraw();
}

void TextureBrowser::onActiveShadersChanged()
{
	heightChanged();
	m_originInvalid = true;
}

// Static command target
void TextureBrowser::toggle(const cmd::ArgumentList& args)
{
	GlobalGroupDialog().togglePage("textures");
}

//++timo NOTE: this is a mix of Shader module stuff and texture explorer
// it might need to be split in parts or moved out .. dunno
// scroll origin so the specified texture is completely on screen
// if current texture is not displayed, nothing is changed
void TextureBrowser::focus(const std::string& name)
{
	// scroll origin so the texture is completely on screen
  
	class FocusWalker :
		public shaders::ShaderVisitor
	{
	private:
		TextureBrowser& _browser;
		TextureLayout _layout;
		int _x;
		int _y;
		const std::string& _name;

	public:
		FocusWalker(TextureBrowser& browser, const std::string& name) :
			_browser(browser),
			_name(name)
		{}

		void visit(const MaterialPtr& shader)
		{
			if (!_browser.shaderIsVisible(shader))
			{
				return;
			}

			_browser.nextTexturePos(_layout, shader->getEditorImage(), &_x, &_y);

			TexturePtr q = shader->getEditorImage();
			
			if (!q) return;

			// we have found when texdef->name and the shader name match
			// NOTE: as everywhere else for our comparisons, we are not case sensitive
			if (shader_equal(_name, shader->getName()))
			{
				int textureHeight = _browser.getTextureHeight(q) + 2 * _browser.getFontHeight();

				int originy = _browser.getOriginY();

				if (_y > originy)
				{
					originy = _y;
				}

				if (_y - textureHeight < originy - _browser.getViewportHeight())
				{
					originy = (_y - textureHeight) + _browser.getViewportHeight();
				}

				_browser.setOriginY(originy);
			}
		}
	} _walker(*this, name);

	GlobalMaterialManager().foreachShader(_walker);
}

MaterialPtr TextureBrowser::getShaderAtCoords(int mx, int my)
{
	my += getOriginY() - height;

	class MaterialFinder :
		public shaders::ShaderVisitor
	{
	private:
		TextureBrowser& _browser;
		TextureLayout _layout;
		int _x;
		int _y;
		MaterialPtr _material;

	public:
		MaterialFinder(TextureBrowser& browser, int x, int y) :
			_browser(browser),
			_x(x),
			_y(y)
		{}

		void visit(const MaterialPtr& shader)
		{
			if (!_browser.shaderIsVisible(shader))
			{
				return;
			}

			int x, y;
			_browser.nextTexturePos(_layout, shader->getEditorImage(), &x, &y);

			TexturePtr tex = shader->getEditorImage();

			if (tex == NULL)
			{
				return;
			}

			int nWidth = _browser.getTextureWidth(tex);
			int nHeight = _browser.getTextureHeight(tex);

			if (_x > x && _x - x < nWidth && _y < y && y - _y < nHeight + _browser.getFontHeight())
			{
				_material = shader;
			}
		}

		MaterialPtr getMaterial()
		{
			return _material;
		}
	} _walker(*this, mx, my);

	GlobalMaterialManager().foreachShader(_walker);

	return _walker.getMaterial();
}

void TextureBrowser::selectTextureAt(int mx, int my)
{
	MaterialPtr shader = getShaderAtCoords(mx, my);

	if (shader != NULL)
	{
  		setSelectedShader(shader->getName());

		// Apply the shader to the current selection
		selection::algorithm::applyShaderToSelection(shader->getName());
	}
}

void TextureBrowser::draw()
{
	GlobalOpenGL().assertNoErrors();

	Vector3 colorBackground = ColourSchemes().getColour("texture_background");
	glClearColor(colorBackground[0], colorBackground[1], colorBackground[2], 0);
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable (GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glOrtho (0, width, originy-height, originy, -100, 100);
	glEnable (GL_TEXTURE_2D);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

	class MaterialRenderer :
		public shaders::ShaderVisitor
	{
	private:
		TextureBrowser& _browser;
		TextureLayout _layout;
		int _lastY;
		int _lastHeight;
		MaterialPtr _material;
		bool _hideUnused;
		unsigned int _maxNameLength;

	public:
		MaterialRenderer(TextureBrowser& browser, bool hideUnused) :
			_browser(browser),
			_lastY(0),
			_lastHeight(0),
			_hideUnused(hideUnused),
			_maxNameLength(registry::getValue<int>(RKEY_TEXTURE_MAX_NAME_LENGTH))
		{}

		void visit(const MaterialPtr& shader)
		{
			if (!_browser.shaderIsVisible(shader))
			{
				return;
			}

			int x, y;
			_browser.nextTexturePos(_layout, shader->getEditorImage(), &x, &y);

			TexturePtr q = shader->getEditorImage();

			if (!q) return;

			int nWidth = _browser.getTextureWidth(q);
			int nHeight = _browser.getTextureHeight(q);

			if (y != _lastY)
			{
				_lastY = y;
				_lastHeight = 0;
			}

			_lastHeight = std::max(nHeight, _lastHeight);

			// Is this texture visible?
			if ((y - nHeight - _browser.getFontHeight() < _browser.getOriginY()) && 
				(y > _browser.getOriginY() - _browser.getViewportHeight()))
			{
				// borders rules:
				// if it's the current texture, draw a thick red line, else:
				// shaders have a white border, simple textures don't
				// if !texture_showinuse: (some textures displayed may not be in use)
				// draw an additional square around with 0.5 1 0.5 color
				if (shader_equal(_browser.getSelectedShader(), shader->getName()))
				{
					glLineWidth(3);
					glColor3f(1,0,0);
					glDisable(GL_TEXTURE_2D);

					glBegin(GL_LINE_LOOP);
					glVertex2i(x - 4, y - _browser.getFontHeight() + 4);
					glVertex2i(x - 4, y - _browser.getFontHeight() - nHeight - 4);
					glVertex2i(x + 4 + nWidth, y - _browser.getFontHeight() - nHeight - 4);
					glVertex2i(x + 4 + nWidth, y - _browser.getFontHeight() + 4);
					glEnd();

					glEnable (GL_TEXTURE_2D);
					glLineWidth (1);
				}
				else
				{
					glLineWidth(1);

					// shader border:
					if (!shader->IsDefault())
					{
						glColor3f(1,1,1);
						glDisable(GL_TEXTURE_2D);

						glBegin(GL_LINE_LOOP);
						glVertex2i(x-1,y+1-_browser.getFontHeight());
						glVertex2i(x-1,y-nHeight-1-_browser.getFontHeight());
						glVertex2i(x+1+nWidth,y-nHeight-1-_browser.getFontHeight());
						glVertex2i(x+1+nWidth,y+1-_browser.getFontHeight());
						glEnd();
						glEnable (GL_TEXTURE_2D);
					}

					// highlight in-use textures
					if (!_hideUnused && shader->IsInUse())
					{
						glColor3f(0.5f, 1 ,0.5f);
						glDisable(GL_TEXTURE_2D);
						glBegin(GL_LINE_LOOP);
						glVertex2i(x-3,y+3-_browser.getFontHeight());
						glVertex2i(x-3,y-nHeight-3-_browser.getFontHeight());
						glVertex2i(x+3+nWidth,y-nHeight-3-_browser.getFontHeight());
						glVertex2i(x+3+nWidth,y+3-_browser.getFontHeight());
						glEnd();
						glEnable (GL_TEXTURE_2D);
					}
				}

				// Draw the texture
				glBindTexture(GL_TEXTURE_2D, q->getGLTexNum());
				GlobalOpenGL().assertNoErrors();
				glColor3f(1,1,1);
				glBegin(GL_QUADS);
				glTexCoord2i(0,0);
				glVertex2i(x,y-_browser.getFontHeight());
				glTexCoord2i(1,0);
				glVertex2i(x+nWidth,y-_browser.getFontHeight());
				glTexCoord2i(1,1);
				glVertex2i(x+nWidth,y-_browser.getFontHeight()-nHeight);
				glTexCoord2i (0,1);
				glVertex2i(x,y-_browser.getFontHeight()-nHeight);
				glEnd();

				// draw the texture name
				glDisable (GL_TEXTURE_2D);
				glColor3f (1,1,1);

				glRasterPos2i (x, y-_browser.getFontHeight()+5);

				// don't draw the directory name
				std::string name = shader->getName();
				name = name.substr(name.rfind("/") + 1);

				// Ellipsize the name if it's too long
				if (name.size() > _maxNameLength)
				{
					name = name.substr(0, _maxNameLength/2) +
      					"..." +
      					name.substr(name.size() - _maxNameLength/2);
				}

				GlobalOpenGL().drawString(name);
				glEnable(GL_TEXTURE_2D);
			}
		}

	} _walker(*this, m_hideUnused);

	GlobalMaterialManager().foreachShader(_walker);

#if 0

  TextureLayout layout;
  for(QERApp_ActiveShaders_IteratorBegin(); !QERApp_ActiveShaders_IteratorAtEnd(); QERApp_ActiveShaders_IteratorIncrement())
  {
    MaterialPtr shader = QERApp_ActiveShaders_IteratorCurrent();

    if (!shaderIsVisible(shader))
      continue;

    int x, y;
    nextTexturePos(layout, shader->getEditorImage(), &x, &y);
    TexturePtr q = shader->getEditorImage();
    if (!q)
      break;

    int nWidth = getTextureWidth(q);
    int nHeight = getTextureHeight(q);

    if (y != last_y)
    {
      last_y = y;
      last_height = 0;
    }
    last_height = std::max (nHeight, last_height);

    // Is this texture visible?
    if ((y - nHeight - getFontHeight() < originy)
        && (y > originy - height))
    {
      // borders rules:
      // if it's the current texture, draw a thick red line, else:
      // shaders have a white border, simple textures don't
      // if !texture_showinuse: (some textures displayed may not be in use)
      // draw an additional square around with 0.5 1 0.5 color
      if (shader_equal(getSelectedShader(), shader->getName()))
      {
	      glLineWidth (3);
	      glColor3f (1,0,0);
	      glDisable (GL_TEXTURE_2D);

	      glBegin (GL_LINE_LOOP);
	      glVertex2i (x-4,y-getFontHeight()+4);
	      glVertex2i (x-4,y-getFontHeight()-nHeight-4);
	      glVertex2i (x+4+nWidth,y-getFontHeight()-nHeight-4);
	      glVertex2i (x+4+nWidth,y-getFontHeight()+4);
	      glEnd();

	      glEnable (GL_TEXTURE_2D);
	      glLineWidth (1);
      }
      else
      {
	      glLineWidth (1);
	      // shader border:
	      if (!shader->IsDefault())
	      {
	        glColor3f (1,1,1);
	        glDisable (GL_TEXTURE_2D);

	        glBegin (GL_LINE_LOOP);
	        glVertex2i (x-1,y+1-getFontHeight());
	        glVertex2i (x-1,y-nHeight-1-getFontHeight());
	        glVertex2i (x+1+nWidth,y-nHeight-1-getFontHeight());
	        glVertex2i (x+1+nWidth,y+1-getFontHeight());
	        glEnd();
	        glEnable (GL_TEXTURE_2D);
	      }

	      // highlight in-use textures
	      if (!m_hideUnused && shader->IsInUse())
	      {
	        glColor3f (0.5,1,0.5);
	        glDisable (GL_TEXTURE_2D);
	        glBegin (GL_LINE_LOOP);
	        glVertex2i (x-3,y+3-getFontHeight());
	        glVertex2i (x-3,y-nHeight-3-getFontHeight());
	        glVertex2i (x+3+nWidth,y-nHeight-3-getFontHeight());
	        glVertex2i (x+3+nWidth,y+3-getFontHeight());
	        glEnd();
	        glEnable (GL_TEXTURE_2D);
	      }
      }

      // Draw the texture
      glBindTexture (GL_TEXTURE_2D, q->getGLTexNum());
      GlobalOpenGL().assertNoErrors();
      glColor3f (1,1,1);
      glBegin (GL_QUADS);
      glTexCoord2i (0,0);
      glVertex2i (x,y-getFontHeight());
      glTexCoord2i (1,0);
      glVertex2i (x+nWidth,y-getFontHeight());
      glTexCoord2i (1,1);
      glVertex2i (x+nWidth,y-getFontHeight()-nHeight);
      glTexCoord2i (0,1);
      glVertex2i (x,y-getFontHeight()-nHeight);
      glEnd();

      // draw the texture name
      glDisable (GL_TEXTURE_2D);
      glColor3f (1,1,1);

      glRasterPos2i (x, y-getFontHeight()+5);

      // don't draw the directory name
      std::string name = shader->getName();
      name = name.substr(name.rfind("/") + 1);

      // Ellipsize the name if it's too long
      if (name.size() > maxNameLength) {
      	name = name.substr(0, maxNameLength/2) +
      		   "..." +
      		   name.substr(name.size() - maxNameLength/2);
      }

      GlobalOpenGL().drawString(name);
      glEnable (GL_TEXTURE_2D);
    }

    //int totalHeight = abs(y) + last_height + textureBrowser.getFontHeight() + 4;
  }
#endif

	// reset the current texture
	glBindTexture(GL_TEXTURE_2D, 0);
    GlobalOpenGL().assertNoErrors();
}

void TextureBrowser::doMouseWheel(bool wheelUp)
{
	int originy = getOriginY();

	if (wheelUp) {
		originy += static_cast<int>(m_mouseWheelScrollIncrement);
	}
	else {
		originy -= static_cast<int>(m_mouseWheelScrollIncrement);
	}

	setOriginY(originy);
}

bool TextureBrowser::checkSeekInMediaBrowser()
{
	if (_popupX > 0 && _popupY > 0)
	{
		MaterialPtr shader = getShaderAtCoords(_popupX, _popupY);

		return (shader != NULL);
	}

	return false;
}

void TextureBrowser::openContextMenu() {

	std::string shaderText = _("No shader");

	if (_popupX > 0 && _popupY > 0)
	{
		MaterialPtr shader = getShaderAtCoords(_popupX, _popupY);

		if (shader != NULL)
		{
			shaderText = shader->getName();
			shaderText = shaderText.substr(shaderText.rfind("/")+1);
		}
	}

	_shaderLabel->set_label(shaderText);

	_popupMenu->show();
}

// Static
void TextureBrowser::onSeekInMediaBrowser()
{
	if (_popupX > 0 && _popupY > 0)
	{
		MaterialPtr shader = getShaderAtCoords(_popupX, _popupY);

		if (shader != NULL)
		{
			// Focus the MediaBrowser selection to the given shader
			GlobalGroupDialog().setPage("mediabrowser");
			MediaBrowser::getInstance().setSelection(shader->getName());
		}
	}

	_popupX = -1;
	_popupY = -1;
}

// GTK callback for toggling uniform texture sizing
void TextureBrowser::onResizeToggle()
{
	m_resizeTextures = _sizeToggle->get_active();

	// Update texture browser
	heightChanged();
}

bool TextureBrowser::onButtonPress(GdkEventButton* ev)
{
	if (ev->type == GDK_BUTTON_PRESS)
	{
		if (ev->button == 3)
		{
			_freezePointer.freeze(_parent, sigc::mem_fun(*this, &TextureBrowser::onFrozenMouseMotion));

			// Store the coords of the mouse pointer for later reference
			_popupX = static_cast<int>(ev->x);
			_popupY = height - 1 - static_cast<int>(ev->y);
			_startOrigin = originy;
		}
		else if (ev->button == 1)
		{
			selectTextureAt(
    			static_cast<int>(ev->x),
    			height - 1 - static_cast<int>(ev->y)
    		);
    	}
	}

	return false;
}

bool TextureBrowser::onButtonRelease(GdkEventButton* ev)
{
	if (ev->type == GDK_BUTTON_RELEASE)
	{
		if (ev->button == 3)
		{
			_freezePointer.unfreeze(_parent);

			// See how much we've been scrolling since mouseDown
			int delta = abs(originy - _startOrigin);

			if (delta <= _epsilon)
			{
				 openContextMenu();
			}

			_startOrigin = -1;
		}
	}

	return false;
}

bool TextureBrowser::onMouseScroll(GdkEventScroll* ev)
{
	if (ev->direction == GDK_SCROLL_UP)
	{
		doMouseWheel(true);
	}
	else if (ev->direction == GDK_SCROLL_DOWN)
	{
		doMouseWheel(false);
	}

	return false;
}

void TextureBrowser::onFrozenMouseMotion(int x, int y, guint state)
{
	if (y != 0)
	{
		int scale = 1;

		if (state & GDK_SHIFT_MASK)
		{
			scale = 4;
		}

		int originy = getOriginY();
		originy += y * scale;

		setOriginY(originy);
	}
}

void TextureBrowser::scrollChanged(double value)
{
	setOriginY(-static_cast<int>(value));
}

void TextureBrowser::updateScroll()
{
	if (m_showTextureScrollbar)
	{
		int totalHeight = getTotalHeight();

		totalHeight = std::max(totalHeight, height);

		Gtk::Adjustment* vadjustment = _textureScrollbar->get_adjustment();

		if (vadjustment != NULL)
		{
			vadjustment->set_value(-getOriginY());
			vadjustment->set_page_size(height);
			vadjustment->set_page_increment(height/2);
			vadjustment->set_step_increment(20);
			vadjustment->set_lower(0);
			vadjustment->set_upper(totalHeight);
		}
	}
}

int TextureBrowser::getViewportHeight()
{
	return height;
}

void TextureBrowser::onSizeAllocate(Gtk::Allocation& allocation)
{
	width = allocation.get_width();
	height = allocation.get_height();
	heightChanged();
	m_originInvalid = true;
	queueDraw();
}

bool TextureBrowser::onExpose(GdkEventExpose* event)
{
	// No widget, no drawing
	if (_glWidget == NULL) return false;

	// This calls glwidget_make_current() for us and swap_buffers at the end of scope
	gtkutil::GLWidgetSentry sentry(*_glWidget);

    GlobalOpenGL().assertNoErrors();
    evaluateHeight();
	updateScroll();
    draw();
    GlobalOpenGL().assertNoErrors();

	return false;
}

Gtk::Widget* TextureBrowser::constructWindow(const Glib::RefPtr<Gtk::Window>& parent)
{
	_parent = parent;

	// Instantiate a new GLwidget without z-buffering
	_glWidget = Gtk::manage(new gtkutil::GLWidget(false, "TextureBrowser"));

	GlobalMaterialManager().addActiveShadersObserver(shared_from_this());

	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 0));

	{
		_vadjustment = Gtk::manage(new gtkutil::DeferredAdjustment(
			boost::bind(&TextureBrowser::scrollChanged, this, _1), 0, 0, 0, 1, 1, 1));
		
		_textureScrollbar = Gtk::manage(new Gtk::VScrollbar);
		_textureScrollbar->set_adjustment(*_vadjustment);
		_textureScrollbar->show();

		hbox->pack_end(*_textureScrollbar, false, true, 0);

		if (m_showTextureScrollbar)
		{
			_textureScrollbar->show();
		}
		else
		{
			_textureScrollbar->hide();
		}
	}

	{
		Gtk::VBox* texbox = Gtk::manage(new Gtk::VBox(false, 0));
		texbox->show();

		hbox->pack_start(*texbox, true, true, 0);

	    // Load the texture toolbar from the registry
	    IToolbarManager& tbCreator = GlobalUIManager().getToolbarManager();
		Gtk::Toolbar* textureToolbar = tbCreator.getToolbar("texture");

	    if (textureToolbar != NULL)
		{
			textureToolbar->show();

			texbox->pack_start(*textureToolbar, false, false, 0);

			// Button for toggling the resizing of textures

			_sizeToggle = Gtk::manage(new Gtk::ToggleToolButton);

			Glib::RefPtr<Gdk::Pixbuf> pixBuf = GlobalUIManager().getLocalPixbuf("texwindow_uniformsize.png");
			Gtk::Image* toggle_image = Gtk::manage(new Gtk::Image(pixBuf));

			_sizeToggle->set_tooltip_text(_("Clamp texture thumbnails to constant size"));
			_sizeToggle->set_label(_("Constant size"));
			_sizeToggle->set_icon_widget(*toggle_image);
			_sizeToggle->set_active(true);

	    	// Insert button and connect callback
			textureToolbar->insert(*_sizeToggle, 0);
			_sizeToggle->signal_toggled().connect(sigc::mem_fun(*this, &TextureBrowser::onResizeToggle));

	    	textureToolbar->show_all();
		}

		{
			_filter = Gtk::manage(new gtkutil::NonModalEntry(
				boost::bind(&TextureBrowser::queueDraw, this),
				boost::bind(&TextureBrowser::clearFilter, this))
			);

			texbox->pack_start(*_filter, false, false, 0);

			if (m_showTextureFilter)
			{
				_filter->show();
			}
			else
			{
				_filter->hide();
			}
		}

		{
			_glWidget->set_events(Gdk::EXPOSURE_MASK | Gdk::BUTTON_PRESS_MASK |
								 Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK);
			_glWidget->set_flags(Gtk::CAN_FOCUS);

			texbox->pack_start(*_glWidget, true, true, 0);
			_glWidget->show();

			_glWidget->signal_size_allocate().connect(sigc::mem_fun(*this, &TextureBrowser::onSizeAllocate));
			_glWidget->signal_expose_event().connect(sigc::mem_fun(*this, &TextureBrowser::onExpose));
			_glWidget->signal_button_press_event().connect(sigc::mem_fun(*this, &TextureBrowser::onButtonPress));
			_glWidget->signal_button_release_event().connect(sigc::mem_fun(*this, &TextureBrowser::onButtonRelease));
			_glWidget->signal_scroll_event().connect(sigc::mem_fun(*this, &TextureBrowser::onMouseScroll));
		}
	}

	updateScroll();
	hbox->unset_focus_chain();

	return hbox;
}

void TextureBrowser::destroyWindow()
{
	GlobalMaterialManager().removeActiveShadersObserver(shared_from_this());

	// Remove the parent reference
	_parent.reset();
}

void TextureBrowser::registerPreferencesPage()
{
	// Add a page to the given group
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage(_("Settings/Texture Browser"));

	// Create the string list containing the texture scalings
	std::list<std::string> textureScaleList;

	textureScaleList.push_back("10%");
	textureScaleList.push_back("25%");
	textureScaleList.push_back("50%");
	textureScaleList.push_back("100%");
	textureScaleList.push_back("200%");

	page->appendCombo(_("Texture Thumbnail Scale"), RKEY_TEXTURE_SCALE, textureScaleList);

	page->appendEntry(_("Uniform texture thumbnail size (pixels)"), RKEY_TEXTURE_UNIFORM_SIZE);
	page->appendCheckBox("", _("Texture scrollbar"), RKEY_TEXTURE_SHOW_SCROLLBAR);
	page->appendEntry(_("Mousewheel Increment"), RKEY_TEXTURE_MOUSE_WHEEL_INCR);
	page->appendSpinner(_("Max shadername length"), RKEY_TEXTURE_MAX_NAME_LENGTH, 4, 100, 1);

	page->appendCheckBox("", _("Show Texture Filter"), RKEY_TEXTURE_SHOW_FILTER);
}

void TextureBrowser::construct()
{
	GlobalEventManager().addRegistryToggle("ShowInUse", RKEY_TEXTURES_HIDE_UNUSED);
	GlobalCommandSystem().addCommand("ViewTextures", TextureBrowser::toggle);
	GlobalEventManager().addCommand("ViewTextures", "ViewTextures");

	TextureBrowser::registerPreferencesPage();
}

void TextureBrowser::destroy()
{
	InstancePtr().reset();
}

TextureBrowser& TextureBrowser::Instance()
{
	TextureBrowserPtr& instancePtr = InstancePtr();

	if (instancePtr == NULL)
	{
		instancePtr.reset(new TextureBrowser);
	}

	return *instancePtr;
}

TextureBrowserPtr& TextureBrowser::InstancePtr()
{
	static TextureBrowserPtr _instance;
	return _instance;
}

void TextureBrowser::update()
{
	heightChanged();
}

} // namespace ui

/** greebo: The accessor method, use this to call non-static TextureBrowser methods
 */
ui::TextureBrowser& GlobalTextureBrowser()
{
	return ui::TextureBrowser::Instance();
}
