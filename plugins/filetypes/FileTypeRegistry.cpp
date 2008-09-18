#include "FileTypeRegistry.h"

#include "os/path.h"
#include "stream/textstream.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>

RadiantFileTypeRegistry::RadiantFileTypeRegistry() {
	addType("*", "*", FileTypePattern("All Files", "*.*"));
}
	
void RadiantFileTypeRegistry::addType(const std::string& moduleType, 
			 const std::string& moduleName, 
			 const FileTypePattern& type)
{
	// Create the association between module name and file type pattern
	ModuleFileType fileType(moduleName, type);
	
	// If there is already a list for this type, add our new type to the
	// back of it
	TypeListMap::iterator i = _typeLists.find(moduleType);
	if (i != _typeLists.end()) {
		i->second->push_back(fileType);
	}
	else {
		// Otherwise create a new type list and add it to our map
		ModuleTypeListPtr newList(new ModuleTypeList());
		newList->push_back(fileType);
		
		_typeLists.insert(TypeListMap::value_type(moduleType, newList));
	}
}
  
ModuleTypeListPtr RadiantFileTypeRegistry::getTypesFor(const std::string& moduleType) {
	
	// Try to find the type list in the map
	TypeListMap::iterator i = _typeLists.find(moduleType);
	if (i != _typeLists.end()) {
		return i->second;
	}
	else {
		// Create a pointer to an empty ModuleTypeList and return this
		// instead of a null shared_ptr
		return ModuleTypeListPtr(new ModuleTypeList());
	}		
}
	
// Look for a module which loads the given extension, by searching under the
// given type category
std::string RadiantFileTypeRegistry::findModuleName(
	const std::string& moduleType, const std::string& extension)
{
	// Convert the file extension to lowercase
	std::string ext = boost::algorithm::to_lower_copy(extension);
	
	// Get the list of types for the type category
	ModuleTypeListPtr list = GlobalFiletypes().getTypesFor(moduleType);
	
	// Search in the list for the given extension
	for (ModuleTypeList::const_iterator i = list->begin();
		 i != list->end();
		 i++)
	{
		std::string patternExt = os::getExtension(i->filePattern.pattern);
		if (patternExt == ext) {
			// Match
			return i->moduleName;	
		}
	}
	
	// Not found, return empty string
	return "";
}
	
// RegisterableModule implementation
const std::string& RadiantFileTypeRegistry::getName() const {
	static std::string _name(MODULE_FILETYPES);
	return _name;
}

const StringSet& RadiantFileTypeRegistry::getDependencies() const {
	static StringSet _dependencies; // no dependencies
	return _dependencies;
}

void RadiantFileTypeRegistry::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "FileTypeRegistry::initialiseModule called.\n";
}

// This will be called by the DarkRadiant main binary's ModuleRegistry
extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(RadiantFileTypeRegistryPtr(new RadiantFileTypeRegistry));
	
	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
