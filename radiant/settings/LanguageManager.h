#ifndef _LANGUAGE_MANAGER_H_
#define _LANGUAGE_MANAGER_H_

#include "imodule.h"

namespace language
{

class LanguageManager;
typedef boost::shared_ptr<LanguageManager> LanguageManagerPtr;

class LanguageManager :
	public RegisterableModule
{
public:
	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
};

} // namespace

#endif /* _LANGUAGE_MANAGER_H_ */
