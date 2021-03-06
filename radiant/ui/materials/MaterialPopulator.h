#pragma once

#include <set>

#include "wxutil/dataview/ThreadedResourceTreePopulator.h"
#include "MaterialTreeView.h"

namespace ui
{

class MaterialPopulator :
    public wxutil::ThreadedResourceTreePopulator
{
private:
    const MaterialTreeView::TreeColumns& _columns;

    // The set of favourites
    std::set<std::string> _favourites;

public:
    // Construct and initialise variables
    MaterialPopulator(const MaterialTreeView::TreeColumns& columns);

    virtual ~MaterialPopulator();

    // Add the given named material to the tree (assuming it was not present before)
    void AddSingleMaterial(const wxutil::TreeModel::Ptr& model, const std::string& materialName);

    // Remove the given named material from the tree (assuming it is present)
    void RemoveSingleMaterial(const wxutil::TreeModel::Ptr& model, const std::string& materialName);

protected:
    virtual void PopulateModel(const wxutil::TreeModel::Ptr& model) override;

    // Special sort algorithm to sort the "Other Materials" separately
    virtual void SortModel(const wxutil::TreeModel::Ptr& model) override;
};

}
