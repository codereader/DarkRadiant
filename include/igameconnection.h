#pragma once

#include "imodule.h"

/// interface for GameConnection main singleton class.
class IGameConnection : public RegisterableModule
{
public:
    //TODO: to be determined...
};

const char* const MODULE_GAMECONNECTION = "GameConnection";

// Accessor method
inline IGameConnection* GlobalGameConnection() 
{
    // Cache the reference locally
    static std::shared_ptr<IGameConnection> _gameConnection(
        std::static_pointer_cast<IGameConnection>(
            module::GlobalModuleRegistry().getModule(MODULE_GAMECONNECTION)
        )
    );
    return _gameConnection.get();
}
