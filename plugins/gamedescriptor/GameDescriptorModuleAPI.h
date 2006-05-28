#ifndef GAMEDESCRIPTORMODULEAPI_H_
#define GAMEDESCRIPTORMODULEAPI_H_

#include "igamedescriptor.h"
#include "generic/constant.h"

/* GameDescriptorModule API wrapper class
 */

class GameDescriptorModuleAPI {

    // Pointer to the implementation class
    IGameDescriptor* _gameDescriptorImpl;

public:

    // Required name constant
    STRING_CONSTANT(Name, "gamedescriptor");

    // Required typedef
    typedef IGameDescriptor Type;

    // Constructor. Initialise the Pimpl here.
    GameDescriptorModuleAPI();

    // Required getTable function, returns a pointer to the implementing class.
    IGameDescriptor* getTable();
};

#endif /*GAMEDESCRIPTORMODULEAPI_H_*/
