#ifndef NAMEOBSERVER_H_
#define NAMEOBSERVER_H_

#include "uniquenames.h"
#include "generic/callback.h"

class NameObserver
{
	UniqueNames& m_names;
	std::string m_name;

	void construct();
	void destroy();

	// Not assignable
	NameObserver& operator=(const NameObserver& other);
public:
	NameObserver(UniqueNames& names);
	NameObserver(const NameObserver& other);
	
	~NameObserver();
	
	bool empty() const;
	
	std::string getName() const;
	
	void nameChanged(const std::string& name);
	typedef MemberCaller1<NameObserver, const std::string&, &NameObserver::nameChanged> NameChangedCaller;
};

#endif /*NAMEOBSERVER_H_*/
