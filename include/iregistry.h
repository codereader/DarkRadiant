#pragma once

#include "imodule.h"
#include "xmlutil/Document.h"
#include "xmlutil/Node.h"

#include <sigc++/slot.h>
#include <sigc++/signal.h>
#include <boost/lexical_cast.hpp>

namespace {
	const std::string RKEY_SKIP_REGISTRY_SAVE = "user/skipRegistrySaveOnShutdown";
}

/**
 * \addtogroup registry XML Registry
 */

// String identifier for the registry module
const std::string MODULE_XMLREGISTRY("XMLRegistry");

/**
 * Abstract base class for the registry module.
 *
 * \ingroup registry
 */
class Registry :
	public RegisterableModule
{
public:
	enum Tree {
		treeStandard,
		treeUser
	};

	// Sets a variable in the XMLRegistry or retrieves one
	virtual void 		set(const std::string& key, const std::string& value) = 0;
	virtual std::string	get(const std::string& key) = 0;

	// Checks whether a key exists in the registry
	virtual bool keyExists(const std::string& key) = 0;

	/**
	 * Import an XML file into the registry, without a version check. If the
	 * file cannot be imported for any reason, a std::runtime_error exception
	 * will be thrown.
	 *
	 * @param importFilePath
	 * Full pathname of the file to import.
	 *
	 * @param parentKey
	 * The path to the node within the current registry under which the
	 * imported nodes should be added.
	 *
	 * @param tree: the tree the file should be imported to (e.g. eDefault)
	 */
	virtual void import(const std::string& importFilePath, const std::string& parentKey, Tree tree) = 0;

	// Dumps the whole XML content to std::out for debugging purposes
	virtual void dump() const = 0;

	// Saves the specified node and all its children into the file <filename>
	virtual void exportToFile(const std::string& key, const std::string& filename = "-") = 0;

	// Retrieves the nodelist corresponding for the specified XPath (wraps to xml::Document)
	virtual xml::NodeList findXPath(const std::string& path) = 0;

	// Creates an empty key
	virtual xml::Node createKey(const std::string& key) = 0;

	// Creates a new node named <key> as children of <path> with the name attribute set to <name>
	// The newly created node is returned after creation
	virtual xml::Node createKeyWithName(const std::string& path,
										const std::string& key,
										const std::string& name) = 0;

	/**
	 * greebo: Sets the named attribute of the given node specified by <path>.
	 *
	 * @path: The XPath to the node.
	 * @attrName: The name of the attribute.
	 * @attrValue: The string value of the attribute.
	 */
	virtual void setAttribute(const std::string& path,
							  const std::string& attrName,
							  const std::string& attrValue) = 0;

	/**
	 * greebo: Loads the value of the given attribute at the given path.
	 *
	 * @path: The XPath to the node.
	 * @attrName: The name of the attribute.
	 *
	 * @returns: the string value of the attribute.
	 */
	virtual std::string getAttribute(const std::string& path, const std::string& attrName) = 0;

	// Deletes an entire subtree from the registry
	virtual void deleteXPath(const std::string& path) = 0;

    /// Return a signal which will be emitted when a given key changes
    virtual sigc::signal<void> signalForKey(const std::string& key) const = 0;
};
typedef boost::shared_ptr<Registry> RegistryPtr;

// This is the accessor for the registry
inline Registry& GlobalRegistry() {
	// Cache the reference locally
	static Registry& _registry(
		*boost::static_pointer_cast<Registry>(
			module::GlobalModuleRegistry().getModule(MODULE_XMLREGISTRY)
		)
	);
	return _registry;
}
