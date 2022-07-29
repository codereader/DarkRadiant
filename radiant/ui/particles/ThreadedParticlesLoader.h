#pragma once

#include "debugging/ScopedDebugTimer.h"

#include "wxutil/dataview/DeclarationTreeView.h"
#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"

namespace ui
{

/**
 * Visitor class to retrieve particle system names and add them to a
 * treemodel.
 */
class ThreadedParticlesLoader final :
    public wxutil::ThreadedDeclarationTreePopulator
{
public:
    ThreadedParticlesLoader(const wxutil::DeclarationTreeView::Columns& columns) :
        ThreadedDeclarationTreePopulator(decl::Type::Particle, columns, "particle16.png")
    {}

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
            auto prtName = def.getDeclName() + ".prt";

            // Add the Def name to the list store
            wxutil::TreeModel::Row row = model->AddItem();

            AssignValuesToRow(row, prtName, def.getDeclName(), prtName, false);
        });
    }
};

}
