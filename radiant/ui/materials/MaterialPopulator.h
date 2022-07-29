#pragma once

#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"
#include "MaterialTreeView.h"
#include "string/string.h"

namespace ui
{

class MaterialPopulator :
    public wxutil::ThreadedDeclarationTreePopulator
{
private:
    const MaterialTreeView::TreeColumns& _columns;

    std::string _otherMaterialsPath;

    wxIcon _folderIcon;
    wxIcon _textureIcon;

    // Maps of names to corresponding treemodel items, for both intermediate
    // paths and explicitly presented paths
    using NamedIterMap = std::map<std::string, wxDataViewItem, string::ILess>;
    NamedIterMap _iters;

public:
    // Construct and initialise variables
    MaterialPopulator(const MaterialTreeView::TreeColumns& columns);

    virtual ~MaterialPopulator();

    // Add the given named material to the tree (assuming it was not present before)
    void AddSingleMaterial(const wxutil::TreeModel::Ptr& model, const std::string& materialName);

    // Remove the given named material from the tree (assuming it is present)
    void RemoveSingleMaterial(const wxutil::TreeModel::Ptr& model, const std::string& materialName);

protected:
    void PopulateModel(const wxutil::TreeModel::Ptr& model) override;

    // Special sort algorithm to sort the "Other Materials" separately
    void SortModel(const wxutil::TreeModel::Ptr& model) override;

private:
    wxutil::TreeModel::Row insertFolder(const wxutil::TreeModel::Ptr& model, const std::string& path,
        const std::string& leafName, const wxDataViewItem& parentItem, bool isOtherMaterial);
    wxutil::TreeModel::Row insertTexture(const wxutil::TreeModel::Ptr& model, const std::string& path,
        const std::string& leafName, const wxDataViewItem& parentItem);
    wxDataViewItem& addRecursive(const wxutil::TreeModel::Ptr& model, const std::string& path, bool isOtherMaterial);
    void insert(const wxutil::TreeModel::Ptr& model, const std::string& name);
};

}
