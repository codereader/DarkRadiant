#pragma once

namespace xmlutil
{

// Called to initialise the libxml2 library
void initModule();

// Shutdown the libxml2 library, freeing allocated #memory
void shutdownModule();

}
