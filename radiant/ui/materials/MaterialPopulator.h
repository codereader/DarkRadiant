#pragma once

#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"
#include "MaterialTreeView.h"

namespace ui
{

class MaterialPopulator :
    public wxutil::ThreadedDeclarationTreePopulator
{
private:
    const MaterialTreeView::TreeColumns& _columns;

    std::string _texturePrefix;
    std::string _otherMaterialsPath;

public:
    // Construct and initialise variables
    MaterialPopulator(const MaterialTreeView::TreeColumns& columns);

    virtual ~MaterialPopulator();

    // Add the given named material to the tree (assuming it was not present before)
    void AddSingleMaterial(const wxutil::TreeModel::Ptr& model, const std::string& materialName);

    // Remove the given named material from the tree (assuming it is present)
    void RemoveSingleMaterial(const wxutil::TreeModel::Ptr& model, const std::string& materialName);

    static std::string GetOtherMaterialsName();

protected:
    void PopulateModel(const wxutil::TreeModel::Ptr& model) override;

    // Special sort algorithm to sort the "Other Materials" separately
    void SortModel(const wxutil::TreeModel::Ptr& model) override;

private:
    wxutil::TreeModel::Row InsertFolder(const wxutil::TreeModel::Ptr& model, const std::string& path,
        const std::string& leafName, const wxDataViewItem& parentItem, bool isOtherMaterial);
    void InsertTexture(const wxutil::TreeModel::Ptr& model, const std::string& path, 
        const std::string& declName, const std::string& leafName, const wxDataViewItem& parentItem);

    void SortModel(const wxutil::TreeModel::Ptr& model, const wxDataViewItem& startItem);
};

}
