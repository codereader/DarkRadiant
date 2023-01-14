#pragma once

#include <string>

class wxWindow;

namespace ui
{

/**
 * Interface of a declaration preview widget.
 * When attached to a DeclarationSelector, it receives word
 * about selection changes to update its previews.
 */
class IDeclarationPreview
{
public:
    virtual ~IDeclarationPreview() {}

    // Returns the widget that can be packed into the selector container
    virtual wxWindow* GetPreviewWidget() = 0;

    // Clear the preview
    virtual void ClearPreview() = 0;

    // Sets the name of the decl that should appear in the preview
    virtual void SetPreviewDeclName(const std::string& declName) = 0;
};

}
