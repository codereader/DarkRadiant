#ifndef NAMESPACE_H_
#define NAMESPACE_H_

#include <map>
#include "inamespace.h"
#include "uniquenames.h"
#include "NameObserver.h"

class BasicNamespace : 
	public Namespace
{
	typedef std::map<NameCallback, NameObserver> Names;
	Names m_names;
	UniqueNames m_uniqueNames;

public:
	// Destructor
	~BasicNamespace();
	
	void attach(const NameCallback& setName, const NameCallbackCallback& attachObserver);
	void detach(const NameCallback& setName, const NameCallbackCallback& detachObserver);

	void makeUnique(const char* name, const NameCallback& setName) const;

	void mergeNames(const BasicNamespace& other) const;
}; // class BasicNamespace

#endif /*NAMESPACE_H_*/
