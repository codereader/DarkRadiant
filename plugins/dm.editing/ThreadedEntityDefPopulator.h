#pragma once

#include <set>

#include "ieclass.h"
#include "ifavourites.h"

#include "wxutil/Bitmap.h"
#include "wxutil/dataview/DeclarationTreeView.h"
#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"
#include "wxutil/dataview/TreeViewItemStyle.h"

namespace ui
{

class ThreadedEntityDefPopulator :
    public wxutil::ThreadedDeclarationTreePopulator
{
private:
    const wxutil::DeclarationTreeView::Columns& _columns;

    wxIcon _icon;

public:
    ThreadedEntityDefPopulator(const wxutil::DeclarationTreeView::Columns& columns, const std::string& iconName) :
        ThreadedDeclarationTreePopulator(decl::Type::EntityDef, columns),
        _columns(columns)
    {
        _icon.CopyFromBitmap(wxutil::GetLocalBitmap(iconName));
    }

    ~ThreadedEntityDefPopulator()
    {
        EnsureStopped();
    }

protected:
    // Predicate to be implemented by subclasses. Returns true if the eclass should be listed.
    virtual bool ClassShouldBeListed(const IEntityClassPtr& eclass) = 0;

    virtual void PopulateModel(const wxutil::TreeModel::Ptr& model) override
    {
        GlobalEntityClassManager().forEachEntityClass([&](const IEntityClassPtr& eclass)
        {
            ThrowIfCancellationRequested();

            // Don't include hidden eclasses
            if (eclass->getVisibility() == vfs::Visibility::HIDDEN) return;

            if (!ClassShouldBeListed(eclass)) return;

            bool isFavourite = IsFavourite(eclass->getDeclName());

            auto row = model->AddItem();

            row[_columns.iconAndName] = wxVariant(wxDataViewIconText(eclass->getDeclName(), _icon));
            row[_columns.iconAndName] = wxutil::TreeViewItemStyle::Declaration(isFavourite);
            row[_columns.fullName] = eclass->getDeclName();
            row[_columns.leafName] = eclass->getDeclName();
            row[_columns.declName] = eclass->getDeclName();
            row[_columns.isFolder] = false;
            row[_columns.isFavourite] = isFavourite;

            row.SendItemAdded();
        });
    }
};

}
