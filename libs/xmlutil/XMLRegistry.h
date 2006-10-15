#ifndef XMLREGISTRY_H_
#define XMLREGISTRY_H_

#include <string>
#include <libxml/parser.h>
#include "Document.h"
#include "Node.h"

namespace xml {

// Constants used in this class
namespace {
	const std::string TOPLEVEL_NODE	= "darkradiant";
	const std::string GLOBALS_NODE 	= "globals"; 
}

class XMLRegistry {
	private:
		// The private pointers to the libxml2 and xmlutil objects 
		xml::Document 	_registry;
		xmlDocPtr		_origXmlDocPtr;
		xmlNodePtr		_importNode;
	public:
		XMLRegistry();
		
		//xml::Document getXmlRegistry();
		void 		setXmlRegistry(const std::string& rawKey, const std::string& value);
		std::string	getXmlRegistry(const std::string& rawKey);
		
		bool keyExists(const std::string&);
		std::string XMLRegistry::preparePath(const std::string&);
		void insertKey(const std::string&);
		
		void addXmlFile(const std::string&);
		
		void dumpXmlRegistry() const;
		
		// Destructor
		~XMLRegistry(); 
};

} // namespace xml

extern xml::XMLRegistry xmlRegistry;

#endif /*XMLREGISTRY_H_*/
