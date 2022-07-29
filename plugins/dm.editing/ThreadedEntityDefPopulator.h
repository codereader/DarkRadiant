#pragma once

#include "ieclass.h"

#include "wxutil/dataview/DeclarationTreeView.h"
#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"

namespace ui
{

class ThreadedEntityDefPopulator :
    public wxutil::ThreadedDeclarationTreePopulator
{
private:
    const wxutil::DeclarationTreeView::Columns& _columns;

public:
    ThreadedEntityDefPopulator(const wxutil::DeclarationTreeView::Columns& columns, const std::string& iconName) :
        ThreadedDeclarationTreePopulator(decl::Type::EntityDef, columns, iconName),
        _columns(columns)
    {}

    ~ThreadedEntityDefPopulator() override
    {
        EnsureStopped();
    }

protected:
    // Predicate to be implemented by subclasses. Returns true if the eclass should be listed.
    virtual bool ClassShouldBeListed(const IEntityClassPtr& eclass) = 0;

    void PopulateModel(const wxutil::TreeModel::Ptr& model) override
    {
        GlobalEntityClassManager().forEachEntityClass([&](const IEntityClassPtr& eclass)
        {
            ThrowIfCancellationRequested();

            // Don't include hidden eclasses
            if (eclass->getVisibility() == vfs::Visibility::HIDDEN) return;

            if (!ClassShouldBeListed(eclass)) return;

            auto row = model->AddItem();
            const auto& declName = eclass->getDeclName();
            AssignValuesToRow(row, declName, declName, declName, false);
        });
    }
};

}
