#pragma once

#include "ifavourites.h"

#include "debugging/ScopedDebugTimer.h"

#include "wxutil/dataview/ResourceTreeView.h"
#include "wxutil/dataview/ThreadedResourceTreePopulator.h"
#include "wxutil/dataview/TreeViewItemStyle.h"

namespace ui
{

/**
 * Visitor class to retrieve particle system names and add them to a
 * treemodel.
 */
class ThreadedParticlesLoader final :
    public wxutil::ThreadedResourceTreePopulator
{
private:
    const wxutil::ResourceTreeView::Columns& _columns;

    std::set<std::string> _favourites;

public:
    ThreadedParticlesLoader(const wxutil::ResourceTreeView::Columns& columns) :
        ThreadedResourceTreePopulator(columns),
        _columns(columns)
    {
        // Get the list of favourites
        _favourites = GlobalFavouritesManager().getFavourites(decl::Type::Particle);
    }

    ~ThreadedParticlesLoader()
    {
        EnsureStopped();
    }

protected:
    void PopulateModel(const wxutil::TreeModel::Ptr& model) override
    {
        ScopedDebugTimer timer("ThreadedParticlesLoader::run()");

        // Create and use a ParticlesVisitor to populate the list
        GlobalParticlesManager().forEachParticleDef([&](const particles::IParticleDef& def)
        {
            ThrowIfCancellationRequested();

            // Add the ".prt" extension to the name fo display in the list
            std::string prtName = def.getName() + ".prt";

            // Add the Def name to the list store
            wxutil::TreeModel::Row row = model->AddItem();

            bool isFavourite = _favourites.count(prtName) > 0;

            row[_columns.iconAndName] = wxVariant(wxDataViewIconText(prtName));
            row[_columns.iconAndName] = wxutil::TreeViewItemStyle::Declaration(isFavourite);
            row[_columns.fullName] = prtName;
            row[_columns.leafName] = prtName;
            row[_columns.isFolder] = false;
            row[_columns.isFavourite] = isFavourite;

            row.SendItemAdded();
        });
    }
};

}
