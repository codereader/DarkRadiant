#include "TextureBrowser.h"

#include "i18n.h"
#include "ieventmanager.h"
#include "itextstream.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "iradiant.h"
#include "ipreferencesystem.h"
#include "imediabrowser.h"

#include "wxutil/menu/IconTextMenuItem.h"
#include "wxutil/GLWidget.h"
#include "wxutil/NonModalEntry.h"
#include "wxutil/MouseButton.h"
#include "wxutil/Modifier.h"

#include "registry/registry.h"
#include "shaderlib.h"
#include "selection/algorithm/Shader.h"
#include "selection/shaderclipboard/ShaderClipboard.h"

#include "string/predicate.h"
#include <functional>

#include <wx/panel.h>
#include <wx/wxprec.h>
#include <wx/toolbar.h>
#include <wx/artprov.h>
#include <wx/scrolbar.h>
#include <wx/sizer.h>

#include "string/case_conv.h"
#include "TextureBrowserManager.h"

namespace ui
{

namespace
{
    const char* const SEEK_IN_MEDIA_BROWSER_TEXT = N_("Seek in Media Browser");
    const char* TEXTURE_ICON = "icon_texture.png";

    int FONT_HEIGHT()
    {
        static int height = GlobalOpenGL().getFontHeight() * 1.15;
        return height;
    }

    const int VIEWPORT_BORDER = 12;
    const int TILE_BORDER = 6;
}

class TextureBrowser::TextureTile
{
private:
    TextureBrowser& _owner;
public:
    Vector2i size;
    Vector2i position;
    MaterialPtr material;

    TextureTile(TextureBrowser& owner) :
        _owner(owner)
    {}

    bool isVisible()
    {
        if (!material)
        {
            return false;
        }

        if (!string::istarts_with(material->getName(), GlobalTexturePrefix_get()))
        {
            return false;
        }

        if (_owner._hideUnused && !material->IsInUse())
        {
            return false;
        }

        if (!_owner.getFilter().empty())
        {
            std::string textureNameCache(material->getName());
            std::string textureName = shader_get_textureName(textureNameCache.c_str()); // can't use temporary material->getName() here

            if (_owner._filterIgnoresTexturePath)
            {
				std::size_t lastSlash = textureName.find_last_of('/');

                if (lastSlash != std::string::npos)
                {
					textureName.erase(0, lastSlash + 1);
                }
            }

			string::to_lower(textureName);

            // case insensitive substring match
            if (textureName.find(string::to_lower_copy(_owner.getFilter())) == std::string::npos)
                return false;
        }

        return true;
    }

    void render()
    {
        if (!_owner.materialIsVisible(material))
        {
            return;
        }

        TexturePtr texture = material->getEditorImage();
        if (!texture) return;

        // Is this texture visible?
        if ((position.y() - size.y() - FONT_HEIGHT() < _owner.getOriginY()) &&
            (position.y() > _owner.getOriginY() - _owner.getViewportHeight()))
        {
            drawBorder();
            drawTextureQuad(texture->getGLTexNum());
            drawTextureName();
        }
    }

private:
    void drawBorder()
    {
        // borders rules:
        // if it's the current texture, draw a thick red line, else:
        // shaders have a white border, simple textures don't
        // if !texture_showinuse: (some textures displayed may not be in use)
        // draw an additional square around with 0.5 1 0.5 color
        if (shader_equal(_owner.getSelectedShader(), material->getName()))
        {
            glLineWidth(3);
            glColor3f(1, 0, 0);
            glDisable(GL_TEXTURE_2D);

            glBegin(GL_LINE_LOOP);
            glVertex2i(position.x() - TILE_BORDER,
                       position.y() - FONT_HEIGHT() + TILE_BORDER);
            glVertex2i(position.x() - TILE_BORDER,
                       position.y() - FONT_HEIGHT() - size.y() - TILE_BORDER);
            glVertex2i(position.x() + TILE_BORDER + size.x(),
                       position.y() - FONT_HEIGHT() - size.y() - TILE_BORDER);
            glVertex2i(position.x() + TILE_BORDER + size.x(),
                       position.y() - FONT_HEIGHT() + TILE_BORDER);
            glEnd();

            glEnable(GL_TEXTURE_2D);
            glLineWidth(1);
        }
        else
        {
            glLineWidth(1);

            // material border:
            if (!material->IsDefault())
            {
                glColor3f(1, 1, 1);
                glDisable(GL_TEXTURE_2D);

                glBegin(GL_LINE_LOOP);
                glVertex2i(position.x() - 1,
                           position.y() + 1 - FONT_HEIGHT());
                glVertex2i(position.x() - 1,
                           position.y() - size.y() - 1 - FONT_HEIGHT());
                glVertex2i(position.x() + 1 + size.x(),
                           position.y() - size.y() - 1 - FONT_HEIGHT());
                glVertex2i(position.x() + 1 + size.x(),
                           position.y() + 1 - FONT_HEIGHT());
                glEnd();
                glEnable(GL_TEXTURE_2D);
            }

            // highlight in-use textures
            if (!_owner._hideUnused && material->IsInUse())
            {
                glColor3f(0.5f, 1, 0.5f);
                glDisable(GL_TEXTURE_2D);
                glBegin(GL_LINE_LOOP);
                glVertex2i(position.x() - 3,
                           position.y() + 3 - FONT_HEIGHT());
                glVertex2i(position.x() - 3,
                           position.y() - size.y() - 3 - FONT_HEIGHT());
                glVertex2i(position.x() + 3 + size.x(),
                           position.y() - size.y() - 3 - FONT_HEIGHT());
                glVertex2i(position.x() + 3 + size.x(),
                           position.y() + 3 - FONT_HEIGHT());
                glEnd();
                glEnable(GL_TEXTURE_2D);
            }
        }
    }

    void drawTextureQuad(GLuint num)
    {
        glBindTexture(GL_TEXTURE_2D, num);
        GlobalOpenGL().assertNoErrors();
        glColor3f(1, 1, 1);

        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex2i(position.x(), position.y() - FONT_HEIGHT());
        glTexCoord2i(1, 0);
        glVertex2i(position.x() + size.x(), position.y() - FONT_HEIGHT());
        glTexCoord2i(1, 1);
        glVertex2i(position.x() + size.x(), position.y() - FONT_HEIGHT() - size.y());
        glTexCoord2i(0, 1);
        glVertex2i(position.x(), position.y() - FONT_HEIGHT() - size.y());
        glEnd();
    }

    void drawTextureName()
    {
        glDisable(GL_TEXTURE_2D);
        glColor3f(1, 1, 1);

        const static int FONT_OFFSET = 6;
        glRasterPos2i(position.x(), position.y() - FONT_HEIGHT() + FONT_OFFSET);

        // don't draw the directory name
        std::string name = material->getName();
        name = name.substr(name.rfind("/") + 1);

        // Ellipsize the name if it's too long
        if (name.size() > _owner._maxNameLength)
        {
            name = name.substr(0, _owner._maxNameLength / 2) +
                "..." +
                name.substr(name.size() - _owner._maxNameLength / 2);
        }

        GlobalOpenGL().drawString(name);
        glEnable(GL_TEXTURE_2D);
    }
};

TextureBrowser::TextureBrowser(wxWindow* parent) :
    wxPanel(parent, wxID_ANY),
    _popupX(-1),
    _popupY(-1),
    _startOrigin(-1),
    _epsilon(registry::getValue<float>(RKEY_TEXTURE_CONTEXTMENU_EPSILON)),
    _popupMenu(new wxutil::PopupMenu),
	_filter(nullptr),
    _filterIgnoresTexturePath(true),
    _filterIsIncremental(true),
    _wxGLWidget(nullptr),
    _heightChanged(true),
    _originInvalid(true),
    _mouseWheelScrollIncrement(registry::getValue<int>(RKEY_TEXTURE_MOUSE_WHEEL_INCR)),
    _showTextureFilter(registry::getValue<bool>(RKEY_TEXTURE_SHOW_FILTER)),
    _showTextureScrollbar(registry::getValue<bool>(RKEY_TEXTURE_SHOW_SCROLLBAR)),
    _hideUnused(registry::getValue<bool>(RKEY_TEXTURES_HIDE_UNUSED)),
    _uniformTextureSize(registry::getValue<int>(RKEY_TEXTURE_UNIFORM_SIZE)),
    _maxNameLength(registry::getValue<int>(RKEY_TEXTURE_MAX_NAME_LENGTH)),
    _updateNeeded(true)
{
    observeKey(RKEY_TEXTURES_HIDE_UNUSED);
    observeKey(RKEY_TEXTURE_UNIFORM_SIZE);
    observeKey(RKEY_TEXTURE_SHOW_SCROLLBAR);
    observeKey(RKEY_TEXTURE_MOUSE_WHEEL_INCR);
    observeKey(RKEY_TEXTURE_SHOW_FILTER);
    observeKey(RKEY_TEXTURE_MAX_NAME_LENGTH);

    _shader = texdef_name_default();

    _shaderLabel = new wxutil::IconTextMenuItem(_("No shader"), TEXTURE_ICON);

    _popupMenu->addItem(_shaderLabel, []() {}, []()->bool { return false; }); // always insensitive

    // Construct the popup context menu
    _seekInMediaBrowser = new wxutil::IconTextMenuItem(_(SEEK_IN_MEDIA_BROWSER_TEXT), 
		TEXTURE_ICON);

    _popupMenu->addItem(_seekInMediaBrowser,
                        std::bind(&TextureBrowser::onSeekInMediaBrowser, this),
                        std::bind(&TextureBrowser::checkSeekInMediaBrowser, this));

	// Catch the RightUp event during mouse capture
	_freezePointer.connectMouseEvents(
		wxutil::FreezePointer::MouseEventFunction(),
		std::bind(&TextureBrowser::onGLMouseButtonRelease, this, std::placeholders::_1));

    GlobalTextureBrowser().registerTextureBrowser(this);

    GlobalMaterialManager().signal_activeShadersChanged().connect(
        sigc::mem_fun(this, &TextureBrowser::onActiveShadersChanged));

    Connect(wxEVT_IDLE, wxIdleEventHandler(TextureBrowser::onIdle), nullptr, this);

    SetSizer(new wxBoxSizer(wxHORIZONTAL));

    wxPanel* texbox = new wxPanel(this, wxID_ANY);
    texbox->SetSizer(new wxBoxSizer(wxVERTICAL));

    // Load the texture toolbar from the registry
    {
        IToolbarManager& tbCreator = GlobalUIManager().getToolbarManager();

        _textureToolbar = tbCreator.getToolbar("texture", texbox);

        if (_textureToolbar != NULL)
        {
            // Pack it into the main window
            texbox->GetSizer()->Add(_textureToolbar, 0, wxEXPAND);
        }
        else
        {
            rWarning() << "TextureBrowser: Cannot instantiate texture toolbar!" << std::endl;
        }
    }

    // Filter text entry
    {
        _filter = new wxutil::NonModalEntry(texbox,
                                            std::bind(&TextureBrowser::queueDraw, this),
                                            std::bind(&TextureBrowser::clearFilter, this),
                                            std::bind(&TextureBrowser::filterChanged, this),
                                            false);

        texbox->GetSizer()->Add(_filter, 0, wxEXPAND);

        if (_showTextureFilter)
        {
            _filter->Show();
        }
        else
        {
            _filter->Hide();
        }
    }

    // GL drawing area
    {
        _wxGLWidget = new wxutil::GLWidget(texbox, std::bind(&TextureBrowser::onRender, this), "TextureBrowser");

        _wxGLWidget->Connect(wxEVT_SIZE, wxSizeEventHandler(TextureBrowser::onGLResize), NULL, this);
        _wxGLWidget->Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(TextureBrowser::onGLMouseScroll), NULL, this);

        _wxGLWidget->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(TextureBrowser::onGLMouseButtonPress), NULL, this);
        _wxGLWidget->Connect(wxEVT_LEFT_DCLICK, wxMouseEventHandler(TextureBrowser::onGLMouseButtonPress), NULL, this);
        _wxGLWidget->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(TextureBrowser::onGLMouseButtonRelease), NULL, this);
        _wxGLWidget->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(TextureBrowser::onGLMouseButtonPress), NULL, this);
        _wxGLWidget->Connect(wxEVT_RIGHT_DCLICK, wxMouseEventHandler(TextureBrowser::onGLMouseButtonPress), NULL, this);
        _wxGLWidget->Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(TextureBrowser::onGLMouseButtonRelease), NULL, this);

        texbox->GetSizer()->Add(_wxGLWidget, 1, wxEXPAND);
    }

    GetSizer()->Add(texbox, 1, wxEXPAND);

    // Scrollbar
    {
        _scrollbar = new wxScrollBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);
        _scrollbar->Connect(wxEVT_SCROLL_CHANGED,
                            wxScrollEventHandler(TextureBrowser::onScrollChanged), NULL, this);
        _scrollbar->Connect(wxEVT_SCROLL_THUMBTRACK,
                            wxScrollEventHandler(TextureBrowser::onScrollChanged), NULL, this);

        GetSizer()->Add(_scrollbar, 0, wxEXPAND);

        if (_showTextureScrollbar)
        {
            _scrollbar->Show();
        }
        else
        {
            _scrollbar->Hide();
        }
    }

    updateScroll();
}

TextureBrowser::~TextureBrowser()
{
    GlobalTextureBrowser().unregisterTextureBrowser(this);

    if (_textureToolbar != nullptr)
    {
        GlobalEventManager().disconnectToolbar(_textureToolbar);
        _textureToolbar = nullptr;
    }
}

void TextureBrowser::observeKey(const std::string& key)
{
    GlobalRegistry().signalForKey(key).connect(
        sigc::mem_fun(this, &TextureBrowser::keyChanged)
    );
}

void TextureBrowser::queueDraw()
{
    if (_wxGLWidget != nullptr)
    {
		_wxGLWidget->Refresh();
    }
}

void TextureBrowser::clearFilter()
{
	_filter->SetValue("");
    queueUpdate();
    queueDraw();
}

void TextureBrowser::filterChanged()
{
    if (_filterIsIncremental)
	{
        queueUpdate();
        queueDraw();
	}
}

void TextureBrowser::keyChanged()
{
    _hideUnused = registry::getValue<bool>(RKEY_TEXTURES_HIDE_UNUSED);
    _showTextureFilter = registry::getValue<bool>(RKEY_TEXTURE_SHOW_FILTER);
    _uniformTextureSize = registry::getValue<int>(RKEY_TEXTURE_UNIFORM_SIZE);
    _showTextureScrollbar = registry::getValue<bool>(RKEY_TEXTURE_SHOW_SCROLLBAR);
    _mouseWheelScrollIncrement = registry::getValue<int>(RKEY_TEXTURE_MOUSE_WHEEL_INCR);
    _maxNameLength = registry::getValue<int>(RKEY_TEXTURE_MAX_NAME_LENGTH);

    if (_showTextureScrollbar)
    {
        _scrollbar->Show();
    }
    else
    {
        _scrollbar->Hide();
    }

    if (_showTextureFilter)
    {
        _filter->Show();
    }
    else
    {
        _filter->Hide();
    }

    queueUpdate();
    _originInvalid = true;
}

// Return the display width of a texture in the texture browser
int TextureBrowser::getTextureWidth(const Texture& tex) const
{
    if (tex.getWidth() >= tex.getHeight())
    {
        // Texture is square, or wider than it is tall
        return _uniformTextureSize;
    }
    else
    {
        // Otherwise, preserve the texture's aspect ratio
        return static_cast<int>(_uniformTextureSize * 
            (static_cast<float>(tex.getWidth()) / tex.getHeight())
        );
    }
}

int TextureBrowser::getTextureHeight(const Texture& tex) const
{
    if (tex.getHeight() >= tex.getWidth())
    {
        // Texture is square, or taller than it is wide
        return _uniformTextureSize;
    }
    else
    {
        // Otherwise, preserve the texture's aspect ratio
        return static_cast<int>(
            _uniformTextureSize
            * (static_cast<float>(tex.getHeight()) / tex.getWidth())
        );
    }
}

const std::string& TextureBrowser::getSelectedShader() const
{
    return _shader;
}

std::string TextureBrowser::getFilter()
{
	return _filter->GetValue().ToStdString();
}

void TextureBrowser::setSelectedShader(const std::string& newShader)
{
    _shader = newShader;
    focus(_shader);
}

// Data structure keeping track of the virtual position for the next texture to
// be drawn in. Only the getPositionForTexture() method should access the values
// in this structure.
class TextureBrowser::CurrentPosition
{
public:

    CurrentPosition()
    : origin(VIEWPORT_BORDER, -VIEWPORT_BORDER), rowAdvance(0)
    { }

    Vector2i origin;
    int rowAdvance;
};

TextureBrowser::Vector2i TextureBrowser::getPositionForTexture(
    CurrentPosition& currentPos, const Texture& tex) const
{
    int nWidth = getTextureWidth(tex);
    int nHeight = getTextureHeight(tex);

    // Wrap to the next row if there is not enough horizontal space for this
    // texture
    if (currentPos.origin.x() + nWidth > _viewportSize.x() - VIEWPORT_BORDER
        && currentPos.rowAdvance != 0)
    {
        currentPos.origin.x() = VIEWPORT_BORDER;
        currentPos.origin.y() -= currentPos.rowAdvance + FONT_HEIGHT() + TILE_BORDER;
        currentPos.rowAdvance = 0;
    }

    // Is our texture larger than the row? If so, grow the row height to match
    // it
    if (currentPos.rowAdvance < nHeight)
    {
        currentPos.rowAdvance = nHeight;
    }

    // Store the coordinates where the texture should be placed
    Vector2i texPos = currentPos.origin;

    // Advance the horizontal position for the next texture
    currentPos.origin.x() += std::max(96, nWidth) + 16;

    return texPos;
}

// if texture_showinuse jump over non in-use textures
bool TextureBrowser::materialIsVisible(const MaterialPtr& material)
{
    if (!material)
    {
        return false;
    }

    if (!string::istarts_with(material->getName(), GlobalTexturePrefix_get()))
    {
        return false;
    }

    if (_hideUnused && !material->IsInUse())
    {
        return false;
    }

    if (!getFilter().empty())
    {
        std::string textureNameCache(material->getName());
        std::string textureName = shader_get_textureName(textureNameCache.c_str()); // can't use temporary material->getName() here

		if (_filterIgnoresTexturePath)
        {
			std::size_t lastSlash = textureName.find_last_of('/');

			if (lastSlash != std::string::npos)
			{
				textureName.erase(0, lastSlash + 1);
			}
        }

		string::to_lower(textureName);

		// case insensitive substring match
		if (textureName.find(string::to_lower_copy(getFilter())) == std::string::npos)
			return false;
    }

    return true;
}

int TextureBrowser::getTotalHeight()
{
    return _entireSpaceHeight;
}

void TextureBrowser::clampOriginY()
{
    if (_viewportOriginY > 0)
    {
        _viewportOriginY = 0;
    }

    int lower = std::min(_viewportSize.y() - getTotalHeight(), 0);

    if (_viewportOriginY < lower)
    {
        _viewportOriginY = lower;
    }
}

int TextureBrowser::getOriginY()
{
    if (_originInvalid)
    {
        _originInvalid = false;
        clampOriginY();
        updateScroll();
    }

    return _viewportOriginY;
}

void TextureBrowser::setOriginY(int newOriginY)
{
    _viewportOriginY = newOriginY;
    clampOriginY();
    updateScroll();
    queueDraw();
}

void TextureBrowser::queueUpdate()
{
    _updateNeeded = true;
}

void TextureBrowser::performUpdate()
{
    _updateNeeded = false;

    // Update all renderable items
    _tiles.clear();

    if (!GlobalMaterialManager().isRealised()) return;

    CurrentPosition layout;
    _entireSpaceHeight = 0;

    GlobalMaterialManager().foreachMaterial([&](const MaterialPtr& mat)
    {
        if (!materialIsVisible(mat))
        {
            return;
        }

        // Create a new tile for this material
        _tiles.push_back(TextureTile(*this));
        TextureTile& tile = _tiles.back();

        tile.material = mat;

        Texture& texture = *tile.material->getEditorImage();

        tile.position = getPositionForTexture(layout, texture);
        tile.size.x() = getTextureWidth(texture);
        tile.size.y() = getTextureHeight(texture);

        _entireSpaceHeight = std::max(
            _entireSpaceHeight,
            abs(tile.position.y()) + FONT_HEIGHT() + tile.size.y() + TILE_BORDER
        );
    });

    updateScroll();
}

void TextureBrowser::onActiveShadersChanged()
{
    queueUpdate();
}

void TextureBrowser::focus(const std::string& name)
{
    for (const TextureTile& tile : _tiles)
    {
        // we have found when texdef->name and the shader name match
        // NOTE: as everywhere else for our comparisons, we are not case sensitive
        if (shader_equal(name, tile.material->getName()))
        {
            // scroll origin so the texture is completely on screen
            int originy = getOriginY();

            if (tile.position.y() > originy)
            {
                originy = tile.position.y();
            }

            if (tile.position.y() - tile.size.y() < originy - getViewportHeight())
            {
                originy = tile.position.y() - tile.size.y() + getViewportHeight();
            }

            setOriginY(originy);
        }
    }
}

MaterialPtr TextureBrowser::getShaderAtCoords(int x, int y)
{
    y += getOriginY() - _viewportSize.y();

    for (const TextureTile& tile : _tiles)
    {
        if (x > tile.position.x() && x - tile.position.x() < tile.size.x() &&
            y < tile.position.y() && tile.position.y() - y < tile.size.y() + FONT_HEIGHT())
        {
            return tile.material;
        }
    }

    return MaterialPtr();
}

void TextureBrowser::selectTextureAt(int mx, int my)
{
    MaterialPtr shader = getShaderAtCoords(mx, my);

    if (shader)
    {
        setSelectedShader(shader->getName());

        // Apply the shader to the current selection
        selection::algorithm::applyShaderToSelection(shader->getName());

		// Copy the shader name to the clipboard too
		GlobalShaderClipboard().setSource(shader->getName());
    }
}

void TextureBrowser::draw()
{
	if (_viewportSize.x() == 0 || _viewportSize.y() == 0)
	{
		return;
	}

	glPushAttrib(GL_ALL_ATTRIB_BITS);

    GlobalOpenGL().assertNoErrors();

    Vector3 colorBackground = ColourSchemes().getColour("texture_background");
    glClearColor(colorBackground[0], colorBackground[1], colorBackground[2], 0);
    glViewport(0, 0, _viewportSize.x(), _viewportSize.y());
    
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable (GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glOrtho(0, _viewportSize.x(), getOriginY() - _viewportSize.y(), getOriginY(), -100, 100);

	GlobalOpenGL().assertNoErrors();

    glEnable (GL_TEXTURE_2D);
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

    for (TextureTile& tile : _tiles)
    {
        tile.render();
    }

	GlobalOpenGL().assertNoErrors();

    // reset the current texture
    glBindTexture(GL_TEXTURE_2D, 0);
    GlobalOpenGL().assertNoErrors();

	glPopAttrib();
}

void TextureBrowser::doMouseWheel(bool wheelUp)
{
    int originy = getOriginY();

    if (wheelUp) {
        originy += static_cast<int>(_mouseWheelScrollIncrement);
    }
    else {
        originy -= static_cast<int>(_mouseWheelScrollIncrement);
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

	_shaderLabel->SetItemLabel(shaderText);

    _popupMenu->show(_wxGLWidget);
}

void TextureBrowser::onSeekInMediaBrowser()
{
    if (_popupX > 0 && _popupY > 0)
    {
        MaterialPtr shader = getShaderAtCoords(_popupX, _popupY);

        if (shader != NULL)
        {
            // Focus the MediaBrowser selection to the given shader
            GlobalGroupDialog().setPage(GlobalMediaBrowser().getGroupDialogTabName());
            GlobalMediaBrowser().setSelection(shader->getName());
        }
    }

    _popupX = -1;
    _popupY = -1;
}

void TextureBrowser::onFrozenMouseMotion(int x, int y, unsigned int state)
{
    if (y != 0)
    {
        int scale = 1;

		if ((state & wxutil::Modifier::SHIFT) != 0)
        {
            scale = 4;
        }

        int originy = getOriginY();
        originy += y * scale;

        setOriginY(originy);
    }
}

void TextureBrowser::onFrozenMouseCaptureLost()
{
	_freezePointer.endCapture();
}

void TextureBrowser::scrollChanged(double value)
{
    setOriginY(-static_cast<int>(value));
}

void TextureBrowser::updateScroll()
{
    if (_showTextureScrollbar)
    {
        int totalHeight = getTotalHeight();

        totalHeight = std::max(totalHeight, _viewportSize.y());

		_scrollbar->SetScrollbar(-getOriginY(), _viewportSize.y(), totalHeight, _viewportSize.y());
    }
}

int TextureBrowser::getViewportHeight()
{
    return _viewportSize.y();
}

void TextureBrowser::onScrollChanged(wxScrollEvent& ev)
{
	setOriginY(-ev.GetPosition());
	queueDraw();
}

void TextureBrowser::onGLResize(wxSizeEvent& ev)
{
	_viewportSize = Vector2i(ev.GetSize().GetWidth(), ev.GetSize().GetHeight());

    queueUpdate();
    queueDraw();

	ev.Skip();
}

void TextureBrowser::onGLMouseScroll(wxMouseEvent& ev)
{
	if (ev.GetWheelRotation() > 0)
	{
		doMouseWheel(true);
	}
	else if (ev.GetWheelRotation() < 0)
	{
		doMouseWheel(false);
	}
}

void TextureBrowser::onGLMouseButtonPress(wxMouseEvent& ev)
{
	if (ev.RightDown())
    {
        _freezePointer.startCapture(_wxGLWidget, 
			std::bind(&TextureBrowser::onFrozenMouseMotion, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
			std::bind(&TextureBrowser::onFrozenMouseCaptureLost, this));

        // Store the coords of the mouse pointer for later reference
        _popupX = static_cast<int>(ev.GetX());
        _popupY = _viewportSize.y() - 1 - static_cast<int>(ev.GetY());
        _startOrigin = _viewportOriginY;
    }
	else if (ev.LeftDown())
    {
        selectTextureAt(
            static_cast<int>(ev.GetX()),
            _viewportSize.y() - 1 - static_cast<int>(ev.GetY())
        );
    }
}

void TextureBrowser::onGLMouseButtonRelease(wxMouseEvent& ev)
{
	if (ev.RightUp())
    {
		_freezePointer.endCapture();

		// See how much we've been scrolling since mouseDown
		int delta = abs(_viewportOriginY - _startOrigin);

		if (delta <= _epsilon)
		{
			openContextMenu();
		}

		_startOrigin = -1;
    }
}

void TextureBrowser::onIdle(wxIdleEvent& ev)
{
    if (_updateNeeded)
    {
        performUpdate();

        if (this->IsShownOnScreen())
        {
            queueDraw();
        }
    }
}

void TextureBrowser::onRender()
{
	GlobalOpenGL().assertNoErrors();

    if (_updateNeeded)
    {
        performUpdate();
    }

    draw();

    GlobalOpenGL().assertNoErrors();
}

} // namespace ui

/** greebo: The accessor method, use this to call non-static TextureBrowser methods
 */
ui::TextureBrowserManager& GlobalTextureBrowser()
{
    return ui::TextureBrowserManager::Instance();
}
