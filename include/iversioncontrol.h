#pragma once

#include "imodule.h"
#include "iarchive.h"

namespace vcs
{

/**
 * Common interface of a version control module offering
 * methods to access its history.
 * 
 * Each module needs to define a unique prefix which are used
 * when resolving item URIs.
 */
class IVersionControlModule
{
public:
    virtual ~IVersionControlModule() {}

    using Ptr = std::shared_ptr<IVersionControlModule>;

    // Returns the prefix which is used to construct URIs
    // that refer to a specific point in the VCS history
    virtual std::string getUriPrefix() = 0;

    // Attempts to open the given resource (in text mode)
    virtual ArchiveTextFilePtr openTextFile(const std::string& vcsUri) = 0;
};


/**
 * Interface which keeps track of the active version control
 * modules of this app.
 * 
 * Version Control modules need to register themselves here
 * to be accessible by the core modules.
 */
class IVersionControlManager :
    public RegisterableModule
{
public:
    virtual ~IVersionControlManager() {}

    // Register the given module. If a module with the same prefix has already been registered,
    // a std::runtime_error exception will be thrown.
    virtual void registerModule(const IVersionControlModule::Ptr& vcsModule) = 0;

    virtual void unregisterModule(const IVersionControlModule::Ptr& vcsModule) = 0;

    // Return the source control module for the given prefix, or an empty reference if nothing found
    virtual IVersionControlModule::Ptr getModuleForPrefix(const std::string& prefix) = 0;
};

constexpr const char* const MODULE_VERSION_CONTROL_MANAGER = "VersionControlManager";

}

// The accessor for the clipper module
inline vcs::IVersionControlManager& GlobalVersionControlManager()
{
    static module::InstanceReference<vcs::IVersionControlManager> _reference(vcs::MODULE_VERSION_CONTROL_MANAGER);
    return _reference;
}
