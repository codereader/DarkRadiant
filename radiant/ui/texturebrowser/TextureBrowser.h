#pragma once

#include <sigc++/connection.h>
#include "iregistry.h"
#include "icommandsystem.h"
#include "wxutil/FreezePointer.h"

#include "texturelib.h"
#include "wxutil/menu/PopupMenu.h"
#include "registry/CachedKey.h"

#include "TextureBrowserManager.h"
#include <wx/panel.h>

namespace wxutil
{
    class NonModalEntry;
    class GLWidget;
}

class wxScrollBar;
class wxScrollEvent;
class wxToolBar;

namespace ui
{

namespace
{
    const char* const RKEY_TEXTURES_HIDE_UNUSED = "user/ui/textures/browser/hideUnused";
    const char* const RKEY_TEXTURES_SHOW_FAVOURITES_ONLY = "user/ui/textures/browser/showFavouritesOnly";
    const char* const RKEY_TEXTURES_SHOW_OTHER_MATERIALS = "user/ui/textures/browser/showOtherMaterials";
    const char* const RKEY_TEXTURES_SHOW_NAMES = "user/ui/textures/browser/showNames";
    const char* const RKEY_TEXTURE_UNIFORM_SIZE = "user/ui/textures/browser/uniformSize";
    const char* const RKEY_TEXTURE_USE_UNIFORM_SCALE = "user/ui/textures/browser/useUniformScale";
    const char* const RKEY_TEXTURE_SCALE = "user/ui/textures/browser/textureScale";
    const char* const RKEY_TEXTURE_SHOW_SCROLLBAR = "user/ui/textures/browser/showScrollBar";
    const char* const RKEY_TEXTURE_MOUSE_WHEEL_INCR = "user/ui/textures/browser/mouseWheelIncrement";
    const char* const RKEY_TEXTURE_SHOW_FILTER = "user/ui/textures/browser/showFilter";
    const char* const RKEY_TEXTURE_CONTEXTMENU_EPSILON = "user/ui/textures/browser/contextMenuMouseEpsilon";
    const char* const RKEY_TEXTURE_MAX_NAME_LENGTH = "user/ui/textures/browser/maxShadernameLength";
}

/**
 * \brief Widget for rendering active textures as tiles in a scrollable
 * container.
 *
 * Uses an OpenGL widget to render a rectangular view into a "virtual space"
 * containing all active texture tiles.
 */
class TextureBrowser :
    public wxPanel,
    public sigc::trackable
{
    class TextureTile;
    typedef std::list<TextureTile> TextureTiles;
    TextureTiles _tiles;

    // Size of the 2D viewport. This is the geometry of the render window, not
    // the entire virtual space.
    Vector2i _viewportSize;

    // Y origin of the virtual space with respect to the current viewport.
    // Starts at zero, then becomes more negative as the view is scrolled
    // downwards.
    int _viewportOriginY;

    // Height of the entire virtual space of texture tiles. This changes when
    // textures are added or removed.
    int _entireSpaceHeight;

    std::string _shader;

    // The coordinates of the point where the mouse button was pressed
    // this is used to check whether the mouse button release is a mouse-drag
    // or a contextmenu open command.
    int _popupX;
    int _popupY;
    int _startOrigin;

    // The maximum distance the mouse pointer may move and still let the context menu
    // pop up on mouse button release
    double _epsilon;

    wxutil::PopupMenuPtr _popupMenu;
    wxMenuItem* _seekInMediaBrowser;
    wxMenuItem* _shaderLabel;

	wxutil::NonModalEntry* _filter;
    bool _filterIgnoresTexturePath;
    bool _filterIsIncremental;

	wxutil::GLWidget* _wxGLWidget;

	wxScrollBar* _scrollbar;

    bool _heightChanged;
    bool _originInvalid;

    wxutil::FreezePointer _freezePointer;

    // the increment step we use against the wheel mouse
    std::size_t _mouseWheelScrollIncrement;

    bool _showTextureFilter;
    // make the texture increments match the grid changes
    bool _showTextureScrollbar;
    // if true, the texture window will only display in-use shaders
    // if false, all the shaders in memory are displayed
    bool _hideUnused;
    bool _showFavouritesOnly;
    registry::CachedKey<bool> _showNamesKey;
    int _textureScale;
    bool _useUniformScale;

    // Cached set of material favourites
    std::set<std::string> _favourites;

    // Whether materials not starting with "textures/" should be visible
    bool _showOtherMaterials;

    // The uniform size (in pixels) that textures are resized to when m_resizeTextures is true.
    int _uniformTextureSize;

    unsigned int _maxNameLength;

	wxToolBar* _textureToolbar;

    // renderable items will be updated next round
    bool _updateNeeded;

public:
    // Constructor
    TextureBrowser(wxWindow* parent);

    virtual ~TextureBrowser();

    // Schedules an update of the renderable items
    void queueUpdate();

    void queueDraw();

    /** greebo: Returns the currently selected shader
     */
    const std::string& getSelectedShader() const;

    /** greebo: Sets the currently selected shader to <newShader> and
     *          refocuses the texturebrowser to that shader.
     */
    void setSelectedShader(const std::string& newShader);

private:
    void clearFilter();

    int getViewportHeight();

    // Callback needed for DeferredAdjustment
    void scrollChanged(double value);

    // Actually updates the renderable items (usually done before rendering)
    void performUpdate();

    // This gets called by the ShaderSystem
    void onActiveShadersChanged();

    // Return the display width/height of a texture in the texture browser
    int getTextureWidth(const Texture& tex) const;
    int getTextureHeight(const Texture& tex) const;

    // Get a new position for the given texture, and advance the CurrentPosition
    // state object.
    class CurrentPosition;
    Vector2i getPositionForTexture(CurrentPosition& layout,
                                   const Texture& texture) const;

    bool checkSeekInMediaBrowser(); // sensitivity check
    void onSeekInMediaBrowser();

    // Displays the context menu
    void openContextMenu();

    void observeKey(const std::string& key);
    void keyChanged();
    void loadScaleFromRegistry();

    void onFavouritesChanged();

    /** greebo: The actual drawing method invoking the GL calls.
     */
    void draw();

    /** greebo: Performs the actual window movement after a mouse scroll.
     */
    void doMouseWheel(bool wheelUp);

    void updateScroll();

    /** greebo: Returns the currently active filter string or "" if
     *          the filter is not active.
     */
    std::string getFilter();

    /**
     * Callback run when filter text was changed.
     */
    void filterChanged();

    /** greebo: Sets the focus of the texture browser to the shader
     *          with the given name.
     */
    void focus(const std::string& name);

    /** greebo: Returns the total height of the GL content
     */
    int getTotalHeight();

    void clampOriginY();

    int getOriginY();
    void setOriginY(int newOriginY);

    /**
     * greebo: Returns the shader at the given coords.
     * The coords are window coords, not the virtual coords
     * which span the entire scrollable area.
     * @returns: the MaterialPtr, which may be empty.
     */
    MaterialPtr getShaderAtCoords(int x, int y);

    /** greebo: Tries to select the shader at the given coords.
     *          When successful, this applies the shader to the
     *          current selection.
     */
    void selectTextureAt(int mx, int my);

    /** greebo: Returns true if the given material is visible,
     * taking filter and showUnused into account.
     */
    bool materialIsVisible(const MaterialPtr& material);

	// wx callbacks
    void onIdle(wxIdleEvent& ev);
	bool onRender();
	void onScrollChanged(wxScrollEvent& ev);
	void onGLResize(wxSizeEvent& ev);
	void onGLMouseScroll(wxMouseEvent& ev);
	void onGLMouseButtonPress(wxMouseEvent& ev);
	void onGLMouseButtonRelease(wxMouseEvent& ev);

    // Called when moving the mouse with the RMB held down (used for scrolling)
    void onFrozenMouseMotion(int x, int y, unsigned int state);
	void onFrozenMouseCaptureLost();
};

} // namespace ui

// Accessor method to the singleton instance
ui::TextureBrowserManager& GlobalTextureBrowser();
