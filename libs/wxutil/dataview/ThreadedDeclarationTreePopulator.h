#pragma once

#include "idecltypes.h"
#include "ifavourites.h"

#include "DeclarationTreeView.h"
#include "ThreadedResourceTreePopulator.h"

namespace wxutil
{

/**
 * Shared ThreadedResourceTreePopulator implementation specialising on populating
 * trees of IDeclaration elements.
 */
class ThreadedDeclarationTreePopulator :
    public ThreadedResourceTreePopulator
{
private:
    const DeclarationTreeView::Columns& _columns;

    std::set<std::string> _favourites;

public:
    ThreadedDeclarationTreePopulator(decl::Type type, const DeclarationTreeView::Columns& columns) :
        ThreadedResourceTreePopulator(columns),
        _columns(columns)
    {
        // Assemble the set of favourites for the given declaration type
        _favourites = GlobalFavouritesManager().getFavourites(decl::getTypeName(type));
    }

    ~ThreadedDeclarationTreePopulator() override
    {
        EnsureStopped();
    }

protected:
    const std::set<std::string>& GetFavourites() const
    {
        return _favourites;
    }

    bool IsFavourite(const std::string& declName)
    {
        return _favourites.count(declName) > 0;
    }
};

}
