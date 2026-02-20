#pragma once

#include <memory>
#include <sigc++/connection.h>
#include "wxutil/decl/DeclarationSelector.h"

// FORWARD DECLS
class Material;
typedef std::shared_ptr<Material> MaterialPtr;

class wxDataViewCtrl;
class wxBitmapToggleButton;

namespace ui
{

class MaterialThumbnailBrowser;

/**
 * A widget that allows the selection of a material. The widget contains
 * three elements - a tree view displaying available materials as
 * identified by the specified prefixes, a TexturePreviewCombo displaying a
 * preview of the currently-selected material and a table containing certain
 * information about it.
 *
 * This widget populates its list of materials automatically, and offers a method
 * that allows calling code to retrieve the user's selection. The set of
 * displayed materials can be defined by passing the corresponding TextureFilter
 * value to the constructor.
 */
class MaterialSelector :
	public wxutil::DeclarationSelector
{
public:
    enum class TextureFilter
    {
        Regular, // prefix: "textures/"
        Lights,  // prefix: light/, fog/
        All,     // Shows all textures, no filter
    };

private:
    TextureFilter _textureFilter;
    MaterialThumbnailBrowser* _thumbnailBrowser;
    wxBitmapToggleButton* _viewToggleBtn;
    bool _showingThumbnails;

    sigc::signal<void()> _selectionChanged;
    sigc::connection _thumbnailSelectionConn;
    sigc::connection _thumbnailActivatedConn;
    sigc::connection _filterTextChangedConn;

public:
    /**
     * @brief Constructor.
     *
     * @param filter Texture set to show in the selector
     */
    MaterialSelector(wxWindow* parent, TextureFilter filter);
    ~MaterialSelector() override;

    // Get the selected Material
    MaterialPtr getSelectedShader();
    std::string GetSelectedDeclName() const override;

    /// Signal emitted when the selection is changed by the user
    sigc::signal<void()> signal_selectionChanged() const { return _selectionChanged; }

    void Populate() override;

protected:
    void onTreeViewSelectionChanged() override;
    bool onTreeViewItemActivated() override;

private:
    void createThumbnailBrowser();
    void createViewToggleButton();
    void connectFilterSignal();
    void switchView(bool showThumbnails);
    void onViewToggle(wxCommandEvent& ev);
    void onThumbnailSelectionChanged();
    void onThumbnailItemActivated();
    void onFilterTextChanged(const std::string& filterText);
};

} // namespace ui
