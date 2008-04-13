#ifndef IREGISTRY_H_
#define IREGISTRY_H_

#include "imodule.h"
#include "xmlutil/Document.h"
#include "xmlutil/Node.h"

namespace {
	const std::string RKEY_SKIP_REGISTRY_SAVE = "user/skipRegistrySaveOnShutdown";
}

/**
 * \addtogroup registry XML Registry
 */

/**
 * \brief Interface for objects which wish to be notified of registry key
 * changes.
 *
 * An object which needs notification when a registry key is changed must
 * implement this interface, and can then be registered with
 * Registry::addKeyObserver() to receive change events for a particular key.
 *
 * \ingroup registry
 */
class RegistryKeyObserver {
public:

    /**
     * @brief Key change notification callback.
     * This method will be invoked when a registry key is changed. Notifications
     * will only occur for keys which were passed to the addKeyObserver() method
     * during registration.
     *
     * @param changedKey
     * The registry key which was changed.
     *
     * @param newValue
     * The new value of the changed key.
     */
	virtual void keyChanged(
        const std::string& changedKey,
        const std::string& newValue
    ) = 0;
};

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
	
	// Loads/saves a floating point value from/to the specified <key>, getFloat returns 0.0f if conversion failed
	virtual float getFloat(const std::string& key) = 0;
	virtual void setFloat(const std::string& key, const double& value) = 0;
	
	// Loads/saves an integer value from/to the specified <key>, getInt returns 0 if conversion failed
	virtual int getInt(const std::string& key) = 0;
	virtual void setInt(const std::string& key, const int& value) = 0;
	
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
	virtual xml::Node createKeyWithName(const std::string& path, const std::string& key, const std::string& name) = 0;
	
	// Deletes an entire subtree from the registry
	virtual void deleteXPath(const std::string& path) = 0;
	
    /**
     * \brief Add a RegistryKeyObserver to receive notifications when the
     * specified key is changed.
     *
     * @param observer
     * The RegistryKeyObserver to be notified of changes.
     *
     * @param observedKey
     * The registry key for which the RegistryKeyObserver should be notified.
     */
	virtual void addKeyObserver(RegistryKeyObserver* observer, const std::string& observedKey) = 0;
	
    /**
     * \brief Remove the given RegistryKeyObserver from the list of notifiable
     * observers.
     *
     * This method prevents the given RegistryKeyObserver from receiving
     * notifications of future registry key changes.
     *
     * @param observer
     * Pointer to a RegistryKeyObserver previously registered with
     * addKeyObserver(). If the RegistryKeyObserver was not previously
     * registered, no action is taken.
     */
	virtual void removeKeyObserver(RegistryKeyObserver* observer) = 0;
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

#endif /*IREGISTRY_H_*/
