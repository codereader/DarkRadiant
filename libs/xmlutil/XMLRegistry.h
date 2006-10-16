#ifndef XMLREGISTRY_H_
#define XMLREGISTRY_H_

#include <string>
#include <libxml/parser.h>
#include "Document.h"
#include "Node.h"

/*	This is the central XMLRegistry structure providing easy methods to store all kinds of
 *  information like globals, toolbar structures and anything that fits into an XML file. 
 * 
 * 	The base structure is automatically created on instantiation and the class in turn
 *  is also created automatically on inclusion of the source file (.cpp)
 *   
 *  Example: store a global variable:
 *  	xmlRegistry.setXmlRegistry("globals/ui/showalllightradii", "1");
 * 
 *  Example: retrieve a global variable 
 *  (this returns "" if the key is not found and an error is written to globalOutputStream):
 *  	std::string value = xmlRegistry.getXmlRegistry("globals/ui/showalllightradii");
 * 
 *  Example: import an XML file into the registry:
 * 		xmlRegistry.addXmlFile(absolute_path_to_file); 
 */

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
		
		// Sets a variable in the XMLRegistry or retrieves one
		void 		setXmlRegistry(const std::string& rawKey, const std::string& value);
		std::string	getXmlRegistry(const std::string& rawKey);
		
		// Checks whether a key exists in the registry
		bool keyExists(const std::string&);
		
		// Adds a whole XML file to the registry
		void addXmlFile(const std::string&);
		
		// Dumps the whole XML content to std::out for debugging purposes
		void dumpXmlRegistry() const;
		
		// Destructor
		~XMLRegistry();
		 
	private:
		std::string XMLRegistry::preparePath(const std::string&);
		void insertKey(const std::string&);
};

} // namespace xml

extern xml::XMLRegistry xmlRegistry;

#endif /*XMLREGISTRY_H_*/
