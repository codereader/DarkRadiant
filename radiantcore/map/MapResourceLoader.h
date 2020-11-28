#pragma once

#include <istream>

#include "imapresource.h"
#include "itextstream.h"
#include "imapformat.h"

#include "infofile/InfoFile.h"
#include "RootNode.h"

namespace map
{

/**
 * A MapResourceLoader is responsible of loading/deserialising
 * the map root node from one or more streams.
 */
class MapResourceLoader
{
private:
    std::istream& _stream;
    const MapFormat& _format;

    // Maps entity,primitive indices to nodes, used in infofile parsing code
    NodeIndexMap _indexMapping;

public:
    MapResourceLoader(std::istream& stream, const MapFormat& format);

    // Process the stream passed to the constructor, returns
    // the root node
    // Throws IMapResource::OperationException on failure or cancel
    RootNodePtr load();

    // Load the info file from the given stream, apply it to the root node
    void loadInfoFile(std::istream& stream, const RootNodePtr& root);
};

}
