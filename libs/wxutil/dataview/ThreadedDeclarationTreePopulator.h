#pragma once

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
public:
    ThreadedDeclarationTreePopulator(const DeclarationTreeView::Columns& columns) :
        ThreadedResourceTreePopulator(columns)
    {}
};

}
