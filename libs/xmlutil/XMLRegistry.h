#ifndef XMLREGISTRY_H_
#define XMLREGISTRY_H_

#include <string>
#include <libxml/parser.h>
#include "Document.h"
#include "Node.h"

/*	This is the central XMLRegistry structure providing easy methods to store all kinds of
 *  information like ui state, toolbar structures and anything that fits into an XML file. 
 * 
 * 	The base structure is automatically created on instantiation and the class in turn
 *  is also created automatically on inclusion of the source file (.cpp)
 *   
 *  Example: store a global variable:
 *  	xmlRegistry.set("globals/ui/showalllightradii", "1");
 * 
 *  Example: retrieve a global variable 
 *  (this returns "" if the key is not found and an error is written to globalOutputStream):
 *  	std::string value = xmlRegistry.get("globals/ui/showalllightradii");
 * 
 *  Example: import an XML file into the registry (note: imported keys overwrite previous ones!) 
 * 		xmlRegistry.importFromFile(absolute_path_to_file[, where_to_import]);
 * 
 *  Example: export a path/key to a file:
 *  	xmlRegistry.exportToFile(node_to_export, absolute_path_to_file);
 */

namespace xml {

// Constants used in this class
namespace {
	const std::string TOPLEVEL_NODE		= "darkradiant";
	const std::string DEFAULT_IMPORT_NODE	= std::string("/") + TOPLEVEL_NODE;
}

class XMLRegistry;
typedef XMLRegistry* XMLRegistryPtr;

class XMLRegistry {
	private:
		// The private pointers to the libxml2 and xmlutil objects 
		xml::Document 	_registry;
		xmlDocPtr		_origXmlDocPtr;
		xmlNodePtr		_importNode;
	public:
		XMLRegistry();
		
		// Sets a variable in the XMLRegistry or retrieves one
		void 		set(const std::string& key, const std::string& value);
		std::string	get(const std::string& key);
		
		// Checks whether a key exists in the registry
		bool keyExists(const std::string& key);
		
		// Adds a whole XML file to the registry
		void importFromFile(const std::string& importFilePath, const std::string& parentKey = DEFAULT_IMPORT_NODE);
		
		// Dumps the whole XML content to std::out for debugging purposes
		void dump() const;
		
		// Saves the specified node and all its children into the file <filename>
		void exportToFile(const std::string& key, const std::string& filename = "-");
		
		// Retrieves the nodelist corresponding for the specified XPath (wraps to xml::Document)
		xml::NodeList findXPath(const std::string& path);
		
		// Destructor
		~XMLRegistry();
		 
	private:
		std::string XMLRegistry::prepareKey(const std::string&);
		void createKey(const std::string&);
};

} // namespace xml

extern xml::XMLRegistry xmlRegistry;

#endif /*XMLREGISTRY_H_*/
