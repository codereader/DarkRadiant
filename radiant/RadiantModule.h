#pragma once

#include "iradiant.h"
#include "messages/CommandExecutionFailed.h"
#include <memory>

namespace radiant
{

/// IRadiant implementation class.
class RadiantModule :
	public IRadiantBase
{
    // Our signals
    sigc::signal<void> _radiantStarted;
    sigc::signal<void> _radiantShutdown;

public:

    /// Broadcast shutdwon signal and clear all listeners
	void broadcastShutdownEvent();

	/// Broadcasts startup signal
	void broadcastStartupEvent();

    // IRadiant implementation
    sigc::signal<void> signal_radiantStarted() const override;
    sigc::signal<void> signal_radiantShutdown() const override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

	// Stuff to be done after the modules have been loaded
	void postModuleInitialisation();
};
typedef std::shared_ptr<RadiantModule> RadiantModulePtr;

/**
 * Return the global Radiant module (for use internally, not by other
 * modules).
 */
RadiantModulePtr getGlobalRadiant();

} // namespace radiant
