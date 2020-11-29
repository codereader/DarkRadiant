#pragma once

#include "MapResource.h"

namespace map
{

class ArchivedMapResource :
    public MapResource
{
public:
    ArchivedMapResource(const std::string& archivePath, const std::string& filePathWithinArchive) :
        MapResource(filePathWithinArchive)
    {}

protected:

};

}
