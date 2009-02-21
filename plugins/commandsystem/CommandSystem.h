#ifndef _COMMANDSYSTEM_H_
#define _COMMANDSYSTEM_H_

#include "icommandsystem.h"

namespace cmd {

class CommandSystem :
	public ICommandSystem
{
public:
	void addCommand(const std::string& name, 
					CommandFunction func, 
					const CommandSignature& signature);

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();
};
typedef boost::shared_ptr<CommandSystem> CommandSystemPtr;

} // namespace cmd

#endif /* _COMMANDSYSTEM_H_ */
