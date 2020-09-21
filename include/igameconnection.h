#pragma once

#include "imodule.h"

/// interface for GameConnection main singleton class.
class IGameConnection : public RegisterableModule
{
public:
    virtual void setCameraSyncEnabled(bool enable) = 0;
    virtual void backSyncCamera() = 0;
    virtual void reloadMap() = 0;
    virtual void setAutoReloadMapEnabled(bool enable) = 0;
    virtual void setUpdateMapLevel(bool on, bool always) = 0;
    virtual void doUpdateMap() = 0;
    virtual void togglePauseGame() = 0;
    virtual void respawnSelectedEntities() = 0;
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
