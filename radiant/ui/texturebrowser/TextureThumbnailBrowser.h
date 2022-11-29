#pragma once

#include "wxutil/FreezePointer.h"

#include "texturelib.h"
#include "wxutil/menu/PopupMenu.h"
#include "registry/CachedKey.h"

#include "wxutil/DockablePanel.h"
#include "wxutil/event/SingleIdleCallback.h"

#include <optional>

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

/**
 * \brief Widget for rendering textures thumbnails as tiles in a scrollable
 * container.
 *
 * Uses an OpenGL widget to render a rectangular view into a "virtual space"
 * containing all active texture tiles.
 */
class TextureThumbnailBrowser :
    public wxPanel,
    public sigc::trackable,
    public wxutil::SingleIdleCallback
{
    class TextureTile;
    typedef std::list<std::shared_ptr<TextureTile>> TextureTiles;
    TextureTiles _tiles;

    // Size of the 2D viewport. This is the geometry of the render window, not
    // the entire virtual space.
    std::optional<Vector2i> _viewportSize;

    // Y origin of the virtual space with respect to the current viewport.
    // Starts at zero, then becomes more negative as the view is scrolled
    // downwards.
    int _viewportOriginY = 0;

    // Height of the entire virtual space of texture tiles. This changes when
    // textures are added or removed.
    int _entireSpaceHeight = 0;

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
    
    registry::CachedKey<bool> _showNamesKey;
    int _textureScale;
    bool _useUniformScale;

    // The uniform size (in pixels) that textures are resized to when m_resizeTextures is true.
    int _uniformTextureSize;

    unsigned int _maxNameLength;

    // renderable items will be updated next round
    bool _updateNeeded;

    // Data structure keeping track of the virtual position for the next texture to
    // be drawn in. Only the getNextPositionForTexture() method should access the values
    // in this structure.
    class CurrentPosition
    {
    public:
        CurrentPosition();

        Vector2i origin;
        int rowAdvance;
    };
    std::unique_ptr<CurrentPosition> _currentPopulationPosition;

public:
    TextureThumbnailBrowser(wxWindow* parent, bool showToolbar = true);

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

protected:
    void onIdle() override;

    // To be implemented by subclasses, this method should iterate over all
    // materials that should be shown in the view, calling createTile on each material.
    // (All previous tiles will already have been removed from the view.)
    virtual void populateTiles() = 0;

    void createTileForMaterial(const MaterialPtr& material);

    // Returns the currently active filter string or "" if not active.
    std::string getFilter();

    // Returns true if the given material name is filtered out (should be invisible)
    bool materialIsFiltered(const std::string& materialName);

    void clearFilter();

private:
    const Vector2i& getViewportSize();
    int getViewportHeight();

    // Callback needed for DeferredAdjustment
    void scrollChanged(double value);

    // Repopulates the texture tiles
    void refreshTiles();

    // Return the display width/height of a texture in the texture browser
    int getTextureWidth(const Texture& tex) const;
    int getTextureHeight(const Texture& tex) const;

    // Get a new position for the given texture, and advance the CurrentPosition
    // state object.
    Vector2i getNextPositionForTexture(const Texture& texture);

    bool checkSeekInMediaBrowser(); // sensitivity check
    void onSeekInMediaBrowser();

    // Displays the context menu
    void openContextMenu();

    void observeKey(const std::string& key);
    void keyChanged();
    void loadScaleFromRegistry();

    /** greebo: The actual drawing method invoking the GL calls.
     */
    void draw();

    /** greebo: Performs the actual window movement after a mouse scroll.
     */
    void doMouseWheel(bool wheelUp);

    void updateScroll();

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

	// wx callbacks
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

