#include "XmlModule.h"

#include <libxml/parser.h>

namespace xmlutil
{

void initModule()
{
    xmlInitParser();
}

// Shutdown the libxml2 library, freeing allocated memory
void shutdownModule()
{
    // The encoding handlers allocate some memory for the name strings, free them
    xmlCleanupParser();
}

}
