#include "NameObserver.h"

NameObserver::NameObserver(UniqueNames& names) : 
	m_names(names)
{
	construct();
}

NameObserver::NameObserver(const NameObserver& other) : 
	m_names(other.m_names), 
	m_name(other.m_name)
{
	construct();
}

NameObserver::~NameObserver() {
	destroy();
}

void NameObserver::construct() {
	if (!empty()) {
		//globalOutputStream() << "construct " << makeQuoted(c_str()) << "\n";
		m_names.insert(name_read(m_name));
	}
}

void NameObserver::destroy() {
	if (!empty()) {
		//globalOutputStream() << "destroy " << makeQuoted(c_str()) << "\n";
		m_names.erase(name_read(m_name));
	}
}

bool NameObserver::empty() const {
	return m_name.empty(); 
}

const char* NameObserver::c_str() const {
	return m_name.c_str();
}

void NameObserver::nameChanged(const std::string& name) {
	destroy();
	m_name = name;
	construct();
}
