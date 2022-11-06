#pragma once

#include "TextureThumbnailBrowser.h"

namespace ui
{
/**
 * Texture thumbnail browser implementation showing all materials
 * used in the current map ("in use").
 */
class MapTextureBrowser :
    public TextureThumbnailBrowser
{
public:
    MapTextureBrowser(wxWindow* parent);
};

}
