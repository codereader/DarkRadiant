#include "TextureThumbnailBrowser.h"

#include "i18n.h"
#include "itextstream.h"
#include "ui/imainframe.h"
#include "ui/itoolbarmanager.h"
#include "ui/iusercontrol.h"
#include "icolourscheme.h"
#include "ifavourites.h"
#include "ishaderclipboard.h"
#include "icommandsystem.h"

#include "wxutil/menu/IconTextMenuItem.h"
#include "wxutil/GLWidget.h"
#include "wxutil/NonModalEntry.h"
#include "wxutil/MouseButton.h"
#include "wxutil/Modifier.h"

#include "registry/registry.h"
#include "shaderlib.h"

#include "string/split.h"
#include "string/case_conv.h"
#include <functional>

#include <wx/panel.h>
#include <wx/wxprec.h>
#include <wx/toolbar.h>
#include <wx/scrolbar.h>
#include <wx/sizer.h>

#include "debugging/gl.h"
#include "ui/mediabrowser/FocusMaterialRequest.h"
#include "TextureBrowserManager.h"

namespace ui
{

namespace
{
    constexpr const char* const SEEK_IN_MEDIA_BROWSER_TEXT = N_("Seek in Media Browser");
    constexpr const char* TEXTURE_ICON = "icon_texture.png";

    int FONT_HEIGHT()
    {
        static int height = GlobalOpenGL().getFontHeight() * 1.15;
        return height;
    }

    constexpr int VIEWPORT_BORDER = 12;
    constexpr int TILE_BORDER = 2;
}

class TextureThumbnailBrowser::TextureTile
{
private:
    TextureThumbnailBrowser& _owner;
public:
    Vector2i size;
    Vector2i position;
    MaterialPtr material;

    TextureTile(TextureThumbnailBrowser& owner) :
        _owner(owner)
    {}

    void render(bool drawName)
    {
        TexturePtr texture = material->getEditorImage();
        if (!texture) return;

        // Is this texture visible?
        if ((position.y() - size.y() - FONT_HEIGHT() < _owner.getOriginY()) &&
            (position.y() > _owner.getOriginY() - _owner.getViewportHeight()))
        {
            drawBorder();
            drawTextureQuad(texture->getGLTexNum());
            if (drawName)
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
            glLineWidth(2);
            glColor3f(0.8f, 0, 0);
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
            if (true && !material->IsDefault())
            {
                glColor3f(0.5f, 0.5f, 0.5f);
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

#if false
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
#endif
        }
    }

    void drawTextureQuad(GLuint num)
    {
        glBindTexture(GL_TEXTURE_2D, num);
        debug::assertNoGlErrors();
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
        glColor3f(0.7f, 0.7f, 0.7f);

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

TextureThumbnailBrowser::TextureThumbnailBrowser(wxWindow* parent, bool showToolbar) :
    wxPanel(parent),
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
    _showNamesKey(RKEY_TEXTURES_SHOW_NAMES),
    _textureScale(50),
    _useUniformScale(registry::getValue<bool>(RKEY_TEXTURE_USE_UNIFORM_SCALE)),
    _uniformTextureSize(registry::getValue<int>(RKEY_TEXTURE_UNIFORM_SIZE)),
    _maxNameLength(registry::getValue<int>(RKEY_TEXTURE_MAX_NAME_LENGTH)),
    _updateNeeded(true)
{
    observeKey(RKEY_TEXTURE_UNIFORM_SIZE);
    observeKey(RKEY_TEXTURE_USE_UNIFORM_SCALE);
    observeKey(RKEY_TEXTURE_SCALE);
    observeKey(RKEY_TEXTURE_SHOW_SCROLLBAR);
    observeKey(RKEY_TEXTURE_MOUSE_WHEEL_INCR);
    observeKey(RKEY_TEXTURE_SHOW_FILTER);
    observeKey(RKEY_TEXTURE_MAX_NAME_LENGTH);
    observeKey(RKEY_TEXTURES_SHOW_NAMES);

    loadScaleFromRegistry();

    _shader = texdef_name_default();

    _shaderLabel = new wxutil::IconTextMenuItem(_("No shader"), TEXTURE_ICON);

    _popupMenu->addItem(_shaderLabel, []() {}, []()->bool { return false; }); // always insensitive

    // Construct the popup context menu
    _seekInMediaBrowser = new wxutil::IconTextMenuItem(_(SEEK_IN_MEDIA_BROWSER_TEXT),
		TEXTURE_ICON);

    _popupMenu->addItem(_seekInMediaBrowser,
                        std::bind(&TextureThumbnailBrowser::onSeekInMediaBrowser, this),
                        std::bind(&TextureThumbnailBrowser::checkSeekInMediaBrowser, this));

	// Catch the RightUp event during mouse capture
	_freezePointer.connectMouseEvents(
		wxutil::FreezePointer::MouseEventFunction(),
		std::bind(&TextureThumbnailBrowser::onGLMouseButtonRelease, this, std::placeholders::_1));

    SetSizer(new wxBoxSizer(wxHORIZONTAL));

    wxPanel* texbox = new wxPanel(this, wxID_ANY);
    texbox->SetSizer(new wxBoxSizer(wxVERTICAL));

    // Load the texture toolbar from the registry
    if (showToolbar)
    {
        auto textureToolbar = GlobalToolBarManager().createToolbar("texture", texbox);

        if (textureToolbar != nullptr)
        {
            // Pack it into the main window
            texbox->GetSizer()->Add(textureToolbar, 0, wxEXPAND);
        }
        else
        {
            rWarning() << "TextureBrowser: Cannot instantiate texture toolbar!" << std::endl;
        }
    }

    // Filter text entry
    {
        _filter = new wxutil::NonModalEntry(texbox,
                                            std::bind(&TextureThumbnailBrowser::queueDraw, this),
                                            std::bind(&TextureThumbnailBrowser::clearFilter, this),
                                            std::bind(&TextureThumbnailBrowser::filterChanged, this),
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
        _wxGLWidget = new wxutil::GLWidget(texbox, std::bind(&TextureThumbnailBrowser::onRender, this), "TextureBrowser");

        _wxGLWidget->Bind(wxEVT_SIZE, &TextureThumbnailBrowser::onGLResize, this);
        _wxGLWidget->Bind(wxEVT_MOUSEWHEEL, &TextureThumbnailBrowser::onGLMouseScroll, this);

        _wxGLWidget->Bind(wxEVT_LEFT_DOWN, &TextureThumbnailBrowser::onGLMouseButtonPress, this);
        _wxGLWidget->Bind(wxEVT_LEFT_DCLICK, &TextureThumbnailBrowser::onGLMouseButtonPress, this);
        _wxGLWidget->Bind(wxEVT_LEFT_UP, &TextureThumbnailBrowser::onGLMouseButtonRelease, this);
        _wxGLWidget->Bind(wxEVT_RIGHT_DOWN, &TextureThumbnailBrowser::onGLMouseButtonPress, this);
        _wxGLWidget->Bind(wxEVT_RIGHT_DCLICK, &TextureThumbnailBrowser::onGLMouseButtonPress, this);
        _wxGLWidget->Bind(wxEVT_RIGHT_UP, &TextureThumbnailBrowser::onGLMouseButtonRelease, this);

        texbox->GetSizer()->Add(_wxGLWidget, 1, wxEXPAND);
    }

    GetSizer()->Add(texbox, 1, wxEXPAND);

    // Scrollbar
    {
        _scrollbar = new wxScrollBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);
        _scrollbar->Bind(wxEVT_SCROLL_CHANGED, &TextureThumbnailBrowser::onScrollChanged, this);
        _scrollbar->Bind(wxEVT_SCROLL_THUMBTRACK, &TextureThumbnailBrowser::onScrollChanged, this);

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

void TextureThumbnailBrowser::loadScaleFromRegistry()
{
    int index = registry::getValue<int>(RKEY_TEXTURE_SCALE);

    switch (index)
    {
    case 0: _textureScale = 5; break;
    case 1: _textureScale = 10; break;
    case 2: _textureScale = 25; break;
    case 3: _textureScale = 50; break;
    case 4: _textureScale = 100; break;
    case 5: _textureScale = 200; break;
    };

    queueDraw();
}

void TextureThumbnailBrowser::observeKey(const std::string& key)
{
    GlobalRegistry().signalForKey(key).connect(
        sigc::mem_fun(this, &TextureThumbnailBrowser::keyChanged)
    );
}

void TextureThumbnailBrowser::queueDraw()
{
    if (_wxGLWidget != nullptr)
    {
		_wxGLWidget->Refresh(false);

#if defined(__WXGTK__) && !wxCHECK_VERSION(3, 1, 3)
        // Just calling Refresh doesn't cut it before wxGTK 3.1.3
        // the GLWidget::OnPaint event is never invoked unless we call Update()
        _wxGLWidget->Update();
#endif
    }
}

void TextureThumbnailBrowser::clearFilter()
{
	_filter->SetValue("");
    queueUpdate();
    queueDraw();
}

void TextureThumbnailBrowser::filterChanged()
{
    if (_filterIsIncremental)
	{
        queueUpdate();
        queueDraw();
	}
}

void TextureThumbnailBrowser::keyChanged()
{
    _showTextureFilter = registry::getValue<bool>(RKEY_TEXTURE_SHOW_FILTER);
    _uniformTextureSize = registry::getValue<int>(RKEY_TEXTURE_UNIFORM_SIZE);
    _useUniformScale = registry::getValue<bool>(RKEY_TEXTURE_USE_UNIFORM_SCALE);
    _showTextureScrollbar = registry::getValue<bool>(RKEY_TEXTURE_SHOW_SCROLLBAR);
    _mouseWheelScrollIncrement = registry::getValue<int>(RKEY_TEXTURE_MOUSE_WHEEL_INCR);
    _maxNameLength = registry::getValue<int>(RKEY_TEXTURE_MAX_NAME_LENGTH);

    loadScaleFromRegistry();

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

    Layout();

    queueUpdate();
    _originInvalid = true;
}

// Return the display width of a texture in the texture browser
int TextureThumbnailBrowser::getTextureWidth(const Texture& tex) const
{
    if (!_useUniformScale)
    {
        // Don't use uniform scale
        return static_cast<int>(tex.getWidth() * (static_cast<float>(_textureScale) / 100));
    }
    else if (tex.getWidth() >= tex.getHeight())
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

int TextureThumbnailBrowser::getTextureHeight(const Texture& tex) const
{
    if (!_useUniformScale)
    {
        // Don't use uniform scale
        return static_cast<int>(tex.getHeight() * (static_cast<float>(_textureScale) / 100));
    }
    else if (tex.getHeight() >= tex.getWidth())
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

const std::string& TextureThumbnailBrowser::getSelectedShader() const
{
    return _shader;
}

std::string TextureThumbnailBrowser::getFilter()
{
	return _filter->GetValue().ToStdString();
}

bool TextureThumbnailBrowser::materialIsFiltered(const std::string& materialName)
{
    auto filterText = getFilter();

    if (filterText.empty()) return false; // not filtered

    std::string textureName = shader_get_textureName(materialName.c_str());

    if (_filterIgnoresTexturePath)
    {
        std::size_t lastSlash = textureName.find_last_of('/');

        if (lastSlash != std::string::npos)
        {
            textureName.erase(0, lastSlash + 1);
        }
    }

    string::to_lower(textureName);

    // Split the filter text into words, every word must match (#5738)
    std::vector<std::string> filters;
    string::split(filters, string::to_lower_copy(filterText), " ");

    // case insensitive substring match (all must match for the name to be visible)
    for (const auto& filter : filters)
    {
        if (textureName.find(filter) == std::string::npos)
        {
            return true; // no match, texture name is filtered out
        }
    }

    return false; // not filtered
}

void TextureThumbnailBrowser::setSelectedShader(const std::string& newShader)
{
    _shader = newShader;
    focus(_shader);
}

TextureThumbnailBrowser::CurrentPosition::CurrentPosition()
: origin(VIEWPORT_BORDER, -VIEWPORT_BORDER), rowAdvance(0)
{ }

Vector2i TextureThumbnailBrowser::getNextPositionForTexture(const Texture& tex)
{
    auto& currentPos = *_currentPopulationPosition;

    int nWidth = getTextureWidth(tex);
    int nHeight = getTextureHeight(tex);

    // Wrap to the next row if there is not enough horizontal space for this
    // texture
    if (currentPos.origin.x() + nWidth > getViewportSize().x() - VIEWPORT_BORDER
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

int TextureThumbnailBrowser::getTotalHeight()
{
    return _entireSpaceHeight;
}

void TextureThumbnailBrowser::clampOriginY()
{
    if (_viewportOriginY > 0)
    {
        _viewportOriginY = 0;
    }

    int lower = std::min(getViewportSize().y() - getTotalHeight(), 0);

    if (_viewportOriginY < lower)
    {
        _viewportOriginY = lower;
    }
}

int TextureThumbnailBrowser::getOriginY()
{
    if (_originInvalid)
    {
        _originInvalid = false;
        clampOriginY();
        updateScroll();
    }

    return _viewportOriginY;
}

void TextureThumbnailBrowser::setOriginY(int newOriginY)
{
    _viewportOriginY = newOriginY;
    clampOriginY();
    updateScroll();
    queueDraw();
}

void TextureThumbnailBrowser::queueUpdate()
{
    _updateNeeded = true;
    requestIdleCallback();
}

void TextureThumbnailBrowser::createTileForMaterial(const MaterialPtr& material)
{
    // Create a new tile for this material
    _tiles.push_back(std::make_shared<TextureTile>(*this));
    auto& tile = *_tiles.back();

    tile.material = material;

    Texture& texture = *tile.material->getEditorImage();

    tile.position = getNextPositionForTexture(texture);
    tile.size.x() = getTextureWidth(texture);
    tile.size.y() = getTextureHeight(texture);

    _entireSpaceHeight = std::max(
        _entireSpaceHeight,
        abs(tile.position.y()) + FONT_HEIGHT() + tile.size.y() + TILE_BORDER
    );
}

void TextureThumbnailBrowser::refreshTiles()
{
    // During startup the openGL module might not have created the font yet
    if (GlobalOpenGL().getFontHeight() == 0)
    {
        return;
    }

    _updateNeeded = false;

    // Update all renderable items
    _tiles.clear();

    _currentPopulationPosition = std::make_unique<CurrentPosition>();
    _entireSpaceHeight = 0;

    populateTiles();

    updateScroll();
    clampOriginY(); // scroll value might be out of range after the update
    _currentPopulationPosition.reset();
}

void TextureThumbnailBrowser::focus(const std::string& name)
{
    if (name.empty())
    {
        return;
    }

    for (const auto& tile : _tiles)
    {
        // we have found when texdef->name and the shader name match
        // NOTE: as everywhere else for our comparisons, we are not case sensitive
        if (shader_equal(name, tile->material->getName()))
        {
            // scroll origin so the texture is completely on screen
            int originy = getOriginY();

            if (tile->position.y() > originy)
            {
                originy = tile->position.y();
            }

            if (tile->position.y() - tile->size.y() < originy - getViewportHeight())
            {
                originy = tile->position.y() - tile->size.y() + getViewportHeight();
            }

            setOriginY(originy);
        }
    }
}

MaterialPtr TextureThumbnailBrowser::getShaderAtCoords(int x, int y)
{
    y += getOriginY() - getViewportSize().y();

    for (const auto& tile : _tiles)
    {
        if (x > tile->position.x() && x - tile->position.x() < tile->size.x() &&
            y < tile->position.y() && tile->position.y() - y < tile->size.y() + FONT_HEIGHT())
        {
            return tile->material;
        }
    }

    return {};
}

void TextureThumbnailBrowser::selectTextureAt(int mx, int my)
{
    if (auto shader = getShaderAtCoords(mx, my); shader)
    {
        setSelectedShader(shader->getName());

        // Apply the shader to the current selection
        GlobalCommandSystem().executeCommand("SetShaderOnSelection", shader->getName());

		// Copy the shader name to the clipboard too
		GlobalShaderClipboard().setSourceShader(shader->getName());
    }
}

void TextureThumbnailBrowser::draw()
{
	if (getViewportSize().x() == 0 || getViewportSize().y() == 0)
	{
		return;
	}

	glPushAttrib(GL_ALL_ATTRIB_BITS);

    debug::assertNoGlErrors();

    Vector3 colorBackground = GlobalColourSchemeManager().getColour("texture_background");
    glClearColor(colorBackground[0], colorBackground[1], colorBackground[2], 0);
    glViewport(0, 0, getViewportSize().x(), getViewportSize().y());

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable (GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glOrtho(0, getViewportSize().x(), getOriginY() - getViewportSize().y(), getOriginY(), -100, 100);

	debug::assertNoGlErrors();

    glEnable (GL_TEXTURE_2D);
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

    for (const auto& tile : _tiles)
    {
        tile->render(_showNamesKey.get());
    }

	debug::assertNoGlErrors();

    // reset the current texture
    glBindTexture(GL_TEXTURE_2D, 0);
    debug::assertNoGlErrors();

	glPopAttrib();
}

void TextureThumbnailBrowser::doMouseWheel(bool wheelUp)
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

bool TextureThumbnailBrowser::checkSeekInMediaBrowser()
{
    if (_popupX > 0 && _popupY > 0)
    {
        MaterialPtr shader = getShaderAtCoords(_popupX, _popupY);

        return (shader != NULL);
    }

    return false;
}

void TextureThumbnailBrowser::openContextMenu() {

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

void TextureThumbnailBrowser::onSeekInMediaBrowser()
{
    if (_popupX > 0 && _popupY > 0)
    {
        if (auto shader = getShaderAtCoords(_popupX, _popupY); shader)
        {
            // Focus the MediaBrowser selection to the given shader
            GlobalCommandSystem().executeCommand(FOCUS_CONTROL_COMMAND, { UserControl::MediaBrowser });
            FocusMaterialRequest::Send(shader->getName());
        }
    }

    _popupX = -1;
    _popupY = -1;
}

void TextureThumbnailBrowser::onFrozenMouseMotion(int x, int y, unsigned int state)
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

void TextureThumbnailBrowser::onFrozenMouseCaptureLost()
{
	_freezePointer.endCapture();
}

void TextureThumbnailBrowser::scrollChanged(double value)
{
    setOriginY(-static_cast<int>(value));
}

void TextureThumbnailBrowser::updateScroll()
{
    if (_showTextureScrollbar)
    {
        int totalHeight = getTotalHeight();

        totalHeight = std::max(totalHeight, getViewportSize().y());

        _scrollbar->SetScrollbar(-getOriginY(), getViewportSize().y(), totalHeight,
                                 getViewportSize().y());
    }
}

int TextureThumbnailBrowser::getViewportHeight()
{
    return getViewportSize().y();
}

void TextureThumbnailBrowser::onScrollChanged(wxScrollEvent& ev)
{
	setOriginY(-ev.GetPosition());
	queueDraw();
}

const Vector2i& TextureThumbnailBrowser::getViewportSize()
{
    if (!_viewportSize) {
        auto size = GetClientSize();
        _viewportSize = Vector2i(size.GetWidth(), size.GetHeight());
    }

    return *_viewportSize;
}

void TextureThumbnailBrowser::onGLResize(wxSizeEvent& ev)
{
	_viewportSize = Vector2i(ev.GetSize().GetWidth(), ev.GetSize().GetHeight());

    queueUpdate();
    queueDraw();

	ev.Skip();
}

void TextureThumbnailBrowser::onGLMouseScroll(wxMouseEvent& ev)
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

void TextureThumbnailBrowser::onGLMouseButtonPress(wxMouseEvent& ev)
{
	if (ev.RightDown())
    {
        _freezePointer.startCapture(_wxGLWidget,
			std::bind(&TextureThumbnailBrowser::onFrozenMouseMotion, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
			std::bind(&TextureThumbnailBrowser::onFrozenMouseCaptureLost, this));

        // Store the coords of the mouse pointer for later reference
        _popupX = static_cast<int>(ev.GetX());
        _popupY = getViewportSize().y() - 1 - static_cast<int>(ev.GetY());
        _startOrigin = _viewportOriginY;
    }
	else if (ev.LeftDown())
    {
        selectTextureAt(
            static_cast<int>(ev.GetX()),
            getViewportSize().y() - 1 - static_cast<int>(ev.GetY())
        );
    }
}

void TextureThumbnailBrowser::onGLMouseButtonRelease(wxMouseEvent& ev)
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

void TextureThumbnailBrowser::onIdle()
{
    if (_updateNeeded)
    {
        refreshTiles();
        queueDraw();
    }
}

bool TextureThumbnailBrowser::onRender()
{
    if (!GlobalMainFrame().screenUpdatesEnabled())
    {
        return false;
    }

	debug::assertNoGlErrors();

    draw();

    debug::assertNoGlErrors();

    return true;
}

} // namespace
