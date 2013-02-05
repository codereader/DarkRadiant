/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#pragma once

#include "iregistry.h"
#include "icommandsystem.h"
#include "gtkutil/NonModalEntry.h"
#include "gtkutil/GLWidget.h"
#include "gtkutil/FreezePointer.h"
#include "gtkutil/DeferredAdjustment.h"
#include "texturelib.h"
#include "gtkutil/menu/PopupMenu.h"
#include <boost/enable_shared_from_this.hpp>

#include <gtkmm/window.h>

namespace Gtk
{
    class MenuItem;
    class Entry;
    class VScrollbar;
    class ToggleToolButton;
}

namespace ui {

class TextureBrowser;
typedef boost::shared_ptr<TextureBrowser> TextureBrowserPtr;

/**
 * \brief
 * Widget for rendering active textures as tiles in a scrollable container.
 *
 * Uses an OpenGL widget to render a rectangular view into a "virtual space"
 * containing all active texture tiles.
 */
class TextureBrowser :
    public sigc::trackable,
    public MaterialManager::ActiveShadersObserver,
    public boost::enable_shared_from_this<TextureBrowser>
{
    typedef BasicVector2<int> Vector2i;

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

    gtkutil::PopupMenuPtr _popupMenu;
    Gtk::MenuItem* _seekInMediaBrowser;
    Gtk::MenuItem* _shaderLabel;

    gtkutil::NonModalEntry* _filter;
    bool _filterIgnoresTexturePath;
    bool _filterIsIncremental;

    Glib::RefPtr<Gtk::Window> _parent;
    gtkutil::GLWidget* _glWidget;

    Gtk::VScrollbar* _textureScrollbar;
    gtkutil::DeferredAdjustment* _vadjustment;

    Gtk::ToggleToolButton* _sizeToggle;

    bool m_heightChanged;
    bool m_originInvalid;
    
    gtkutil::FreezePointer _freezePointer;
    
    // the increment step we use against the wheel mouse
    std::size_t m_mouseWheelScrollIncrement;
    std::size_t m_textureScale;
    bool m_showTextureFilter;
    // make the texture increments match the grid changes
    bool m_showTextureScrollbar;
    // if true, the texture window will only display in-use shaders
    // if false, all the shaders in memory are displayed
    bool m_hideUnused;
    
    // If true, textures are resized to a uniform size when displayed in the texture browser.
    // If false, textures are displayed in proportion to their pixel size.
    bool m_resizeTextures;
    // The uniform size (in pixels) that textures are resized to when m_resizeTextures is true.
    int m_uniformTextureSize;
    
public:
    // Constructor
    TextureBrowser();

    // Triggers a refresh
    void update();

    void clearFilter();

    int getViewportHeight();

    /**
     * greebo: Constructs the TextureBrowser window and retrieves the
     * widget for packing into the GroupDialog for instance.
     */
    Gtk::Widget* constructWindow(const Glib::RefPtr<Gtk::Window>& parent);
    void destroyWindow();

    void queueDraw();

    // Callback needed for DeferredAdjustment
    void scrollChanged(double value);

    /** greebo: Returns the currently selected shader
     */
    const std::string& getSelectedShader() const;

    /** greebo: Sets the currently selected shader to <newShader> and
     *          refocuses the texturebrowser to that shader.
     */
    void setSelectedShader(const std::string& newShader);

    /** greebo: This toggles the display of the TextureBrowser,
     *          basically passes the call to the GroupDialog instance,
     *          which takes care of the details.
     *
     * Note: This is a command target, hence the static
     */
    static void toggle(const cmd::ArgumentList& args);

    /** greebo: Adds the according options to the Preferences dialog.
     */
    static void registerPreferencesPage();

    static void construct();
    static void destroy();

    // Accessor to the singleton
    static TextureBrowser& Instance();

private:
    static TextureBrowserPtr& InstancePtr();

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

    // greebo: This gets called as soon as the texture mode gets changed
    void textureModeChanged();

    void observeKey(const std::string& key);
    void keyChanged();

    void setScaleFromRegistry();

    /** greebo: The actual drawing method invoking the GL calls.
     */
    void draw();

    /** greebo: Performs the actual window movement after a mouse scroll.
     */
    void doMouseWheel(bool wheelUp);

    void heightChanged();

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

    void evaluateHeight();

    /** greebo: Returns the total height of the GL content
     */
    int getTotalHeight();

    void clampOriginY();

    int getOriginY();
    void setOriginY(int newOriginY);

public:
    // This gets called by the ShaderSystem
    void onActiveShadersChanged();

private:
    /** greebo: Returns the shader at the given coords.
     *
     * @returns: the MaterialPtr, which may be empty.
     */
    MaterialPtr getShaderAtCoords(int mx, int my);

    /** greebo: Tries to select the shader at the given coords.
     *          When successful, this applies the shader to the
     *          current selection.
     */
    void selectTextureAt(int mx, int my);

    /** greebo: Returns true if the given <shader> is visible,
     *          taking filter and showUnused into account.
     */
    bool shaderIsVisible(const MaterialPtr& shader);

    // gtkmm Callbacks
    bool onExpose(GdkEventExpose* ev);
    void onSizeAllocate(Gtk::Allocation& allocation);
    void onResizeToggle();
    
    // gtkmm Mouse Event Callbacks
    bool onButtonPress(GdkEventButton* ev);
    bool onButtonRelease(GdkEventButton* ev);
    bool onMouseScroll(GdkEventScroll* ev);

    // Called when moving the mouse with the RMB held down (used for scrolling)
    void onFrozenMouseMotion(int x, int y, guint state);
};

} // namespace ui

// Accessor method to the singleton instance
ui::TextureBrowser& GlobalTextureBrowser();
