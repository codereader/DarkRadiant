#pragma once

namespace ui
{
/**
 * Adds and maintains the main menu items to toggle/focus the registered controls
 * like Media Browser, EntityInspector, Texture Tool, Surface Inspector, etc.
 *
 * Adds items on construction, removes the menu items on destruction.
 */
class ViewMenu final
{
public:
    ViewMenu();
    ~ViewMenu();

    void refresh();
};

}
