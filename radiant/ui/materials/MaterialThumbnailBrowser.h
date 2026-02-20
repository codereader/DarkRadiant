#pragma once

#include <sigc++/signal.h>
#include "../texturebrowser/TextureThumbnailBrowser.h"

namespace ui
{

/**
 * Thumbnail browser for the MaterialChooser dialog.
 * Emits signals on selection rather than applying materials directly.
 */
class MaterialThumbnailBrowser :
    public TextureThumbnailBrowser
{
public:
    enum class TextureFilter
    {
        Regular,
        Lights,
        All,
    };

private:
    TextureFilter _textureFilter;
    std::vector<std::string> _prefixes;
    std::string _externalFilter;

    sigc::signal<void()> _signalSelectionChanged;
    sigc::signal<void()> _signalItemActivated;

public:
    MaterialThumbnailBrowser(wxWindow* parent, TextureFilter filter);

    sigc::signal<void()>& signal_selectionChanged();
    sigc::signal<void()>& signal_itemActivated();

    void setExternalFilterText(const std::string& filter);

protected:
    void populateTiles() override;
    void handleMaterialSelection(const MaterialPtr& material) override;
    void handleMaterialActivated(const MaterialPtr& material) override;
};

} // namespace ui
