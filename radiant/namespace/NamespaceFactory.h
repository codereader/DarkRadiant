#ifndef _NAMESPACE_FACTORY_H__
#define _NAMESPACE_FACTORY_H__

#include "inamespace.h"

class NamespaceFactory :
	public INamespaceFactory
{
public:
	/** 
	 * Creates and returns a new Namespace.
	 */
	virtual INamespacePtr createNamespace();

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};

#endif /* _NAMESPACE_FACTORY_H__ */
