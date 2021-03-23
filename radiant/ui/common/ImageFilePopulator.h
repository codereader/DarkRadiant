#pragma once

#include "ifilesystem.h"
#include "wxutil/dataview/ThreadedResourceTreePopulator.h"
#include "wxutil/dataview/ResourceTreeView.h"
#include "wxutil/dataview/VFSTreePopulator.h"

#include "ImageFileSelector.h"

namespace ui
{

class ImageFilePopulator final :
    public wxutil::ThreadedResourceTreePopulator
{
private:
    const ImageFileSelector::Columns& _columns;
    const char* const GKEY_IMAGE_TYPES = "/filetypes/texture//extension";
    std::set<std::string> _extensions;

public:
    // Construct and initialise variables
    ImageFilePopulator(const ImageFileSelector::Columns& columns);

    ~ImageFilePopulator();

protected:
    void PopulateModel(const wxutil::TreeModel::Ptr& model) override;
    void SortModel(const wxutil::TreeModel::Ptr& model) override;
};

}
