#ifndef NAMESPACEMODULE_H_
#define NAMESPACEMODULE_H_

#include "Namespace.h"
#include <boost/shared_ptr.hpp>

class NamespaceAPI
{
	typedef boost::shared_ptr<Namespace> NamespacePtr;
	NamespacePtr _namespace;
public:
	typedef INamespace Type;
	STRING_CONSTANT(Name, "*");

	NamespaceAPI();
	
	INamespace* getTable();
};

#endif /*NAMESPACEMODULE_H_*/
