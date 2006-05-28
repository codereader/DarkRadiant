#include "GameDescriptorModuleAPI.h"

#include "XMLGameDescriptor.h"

// Constructor

GameDescriptorModuleAPI::GameDescriptorModuleAPI():
    _gameDescriptorImpl(new XMLGameDescriptor())
{
    
}

// Required getTable function. Return the Pimpl.

IGameDescriptor* GameDescriptorModuleAPI::getTable() {
    return _gameDescriptorImpl;
}
