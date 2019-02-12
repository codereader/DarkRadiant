#include "EClassManager.h"

#include "iarchive.h"
#include "i18n.h"
#include "iregistry.h"
#include "irender.h"
#include "ieventmanager.h"
#include "icommandsystem.h"
#include "imainframe.h"
#include "iradiant.h"
#include "iuimanager.h"
#include "ifilesystem.h"
#include "parser/DefTokeniser.h"

#include "Doom3EntityClass.h"
#include "Doom3ModelDef.h"

#include "string/case_conv.h"
#include <functional>

#include "debugging/ScopedDebugTimer.h"
#include "modulesystem/StaticModule.h"

namespace eclass {

// Constructor
EClassManager::EClassManager() :
    _realised(false),
    _defLoader(std::bind(&EClassManager::loadDefAndResolveInheritance, this)),
	_curParseStamp(0)
{}

sigc::signal<void> EClassManager::defsReloadedSignal() const
{
    return _defsReloadedSignal;
}

// Get a named entity class, creating if necessary
IEntityClassPtr EClassManager::findOrInsert(const std::string& name, bool has_brushes)
{
    ensureDefsLoaded();

    // Return an error if no name is given
    if (name.empty())
    {
        return IEntityClassPtr();
    }

	// Convert string to lowercase, for case-insensitive lookup
	std::string lName = string::to_lower_copy(name);

    // Find and return if exists
    Doom3EntityClassPtr eclass = findInternal(lName);
    if (eclass)
    {
        return eclass;
    }

    // Otherwise insert the new EntityClass
    //IEntityClassPtr eclass = eclass::Doom3EntityClass::create(lName, has_brushes);
    // greebo: Changed fallback behaviour when unknown entites are encountered to TRUE
    // so that brushes of unknown entites don't get lost (issue #240)
    eclass = Doom3EntityClass::create(lName, true);

    // Try to insert the class
    return insertUnique(eclass);
}

Doom3EntityClassPtr EClassManager::findInternal(const std::string& name)
{
    // Find the EntityClass in the map.
    EntityClasses::const_iterator i = _entityClasses.find(name);

    return i != _entityClasses.end() ? i->second : Doom3EntityClassPtr();
}

Doom3EntityClassPtr EClassManager::insertUnique(const Doom3EntityClassPtr& eclass)
{
	// Try to insert the eclass
    std::pair<EntityClasses::iterator, bool> i = _entityClasses.insert(
    	EntityClasses::value_type(eclass->getName(), eclass)
    );

    // Return the pointer to the inserted eclass
    return i.first->second;
}

void EClassManager::resolveModelInheritance(const std::string& name, const Doom3ModelDefPtr& model)
{
	if (model->resolved == true) {
		return; // inheritance already resolved
	}

	model->resolved = true;

	if (!model->parent.empty())
	{
		Models::iterator i = _models.find(model->parent);

		if (i == _models.end()) {
			rError() << "model " << name
				<< " inherits unknown model " << model->parent << std::endl;
		}
		else {
			resolveModelInheritance(i->first, i->second);

			// greebo: Only inherit the "mesh" of the parent if the current declaration doesn't have one
			if (model->mesh.empty())
			{
				model->mesh = i->second->mesh;
			}

			// Only inherit the "skin" of the parent if the current declaration doesn't have one
			if (model->skin.empty())
			{
				model->skin = i->second->skin;
			}

			// Append all inherited animations, if missing on the child
			model->anims.insert(i->second->anims.begin(), i->second->anims.end());
		}
	}
}

void EClassManager::parseDefFiles()
{
	rMessage() << "searching vfs directory 'def' for *.def\n";

	// Increase the parse stamp for this run
	_curParseStamp++;

	{
		ScopedDebugTimer timer("EntityDefs parsed: ");
        GlobalFileSystem().forEachFile(
            "def/", "def",
            [&](const vfs::FileInfo& fileInfo) { parseFile(fileInfo.name); }
        );
	}
}

void EClassManager::resolveInheritance()
{
	// Resolve inheritance on the model classes
    for (Models::value_type& pair : _models)
    {
    	resolveModelInheritance(pair.first, pair.second);
    }

    // Resolve inheritance for the entities. At this stage the classes
    // will have the name of their parent, but not an actual pointer to
    // it
    for (EntityClasses::value_type& pair : _entityClasses)
	{
		// Tell the class to resolve its own inheritance using the given
		// map as a source for parent lookup
        pair.second->resolveInheritance(_entityClasses);

        // If the entity has a model path ("model" key), lookup the actual
        // model and apply its mesh and skin to this entity.
        if (!pair.second->getModelPath().empty())
        {
            Models::iterator j = _models.find(pair.second->getModelPath());

            if (j != _models.end())
            {
                pair.second->setModelPath(j->second->mesh);
                pair.second->setSkin(j->second->skin);
            }
        }
    }

	// greebo: Override the eclass colours of two special entityclasses
    Vector3 worlspawnColour = ColourSchemes().getColour("default_brush");
    Vector3 lightColour = ColourSchemes().getColour("light_volumes");

    Doom3EntityClassPtr light = findInternal("light");

	if (light)
	{
		light->setColour(lightColour);
	}

	Doom3EntityClassPtr worldspawn = findInternal("worldspawn");

	if (worldspawn)
	{
		worldspawn->setColour(worlspawnColour);
	}
}

void EClassManager::ensureDefsLoaded()
{
    assert(_realised);

    _defLoader.ensureFinished();
}

void EClassManager::loadDefAndResolveInheritance()
{
    parseDefFiles();
    resolveInheritance();
}

void EClassManager::realise()
{
	if (_realised) 
    {
		return; // nothing to do anymore
	}

	_realised = true;

    _defLoader.start();
}

// Find an entity class
IEntityClassPtr EClassManager::findClass(const std::string& className)
{
    ensureDefsLoaded();

	// greebo: Convert the lookup className string to lowercase first
	std::string classNameLower = string::to_lower_copy(className);

    EntityClasses::const_iterator i = _entityClasses.find(classNameLower);

    return i != _entityClasses.end() ? i->second : IEntityClassPtr();
}

// Visit each entity class
void EClassManager::forEachEntityClass(EntityClassVisitor& visitor)
{
    ensureDefsLoaded();

	for (EntityClasses::value_type& pair : _entityClasses)
	{
		visitor.visit(pair.second);
	}
}

void EClassManager::unrealise()
{
    if (_realised)
	{
        // This waits for any threaded work to finish
        _defLoader.reset();
       	_realised = false;
    }
}

IModelDefPtr EClassManager::findModel(const std::string& name)
{
    ensureDefsLoaded();

	Models::const_iterator found = _models.find(name);
	return (found != _models.end()) ? found->second : Doom3ModelDefPtr();
}

void EClassManager::forEachModelDef(ModelDefVisitor& visitor)
{
    ensureDefsLoaded();

	for (const Models::value_type& pair : _models)
	{
		visitor.visit(pair.second);
	}
}

void EClassManager::reloadDefs()
{
	// greebo: Leave all current entityclasses as they are, just invoke the
	// FileLoader again. It will parse the files again, and look up
	// the eclass names in the existing map. If found, the eclass
	// will be asked to clear itself and re-parse from the tokens.
	// This is to assure that any IEntityClassPtrs remain intact during
	// the process, only the class contents change.
	parseDefFiles();

	// Resolve the eclass inheritance again
	resolveInheritance();

    _defsReloadedSignal.emit();
}

// RegisterableModule implementation
const std::string& EClassManager::getName() const {
	static std::string _name(MODULE_ECLASSMANAGER);
	return _name;
}

const StringSet& EClassManager::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_RENDERSYSTEM);
		_dependencies.insert(MODULE_UIMANAGER);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
	}

	return _dependencies;
}

void EClassManager::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "EntityClassDoom3::initialiseModule called." << std::endl;

	GlobalFileSystem().addObserver(*this);
	realise();

	GlobalCommandSystem().addCommand("ReloadDefs", std::bind(&EClassManager::reloadDefsCmd, this, std::placeholders::_1));
	GlobalEventManager().addCommand("ReloadDefs", "ReloadDefs");
}

void EClassManager::shutdownModule()
{
	rMessage() << "EntityClassDoom3::shutdownModule called." << std::endl;

	GlobalFileSystem().removeObserver(*this);

	// Unrealise ourselves and wait for threads to finish
	unrealise();
	
	// Don't notify anyone anymore
	_defsReloadedSignal.clear();

	// Clear member structures
	_entityClasses.clear();
	_models.clear();
}

// This takes care of relading the entityDefs and refreshing the scenegraph
void EClassManager::reloadDefsCmd(const cmd::ArgumentList& args)
{
    IScopedScreenUpdateBlockerPtr blocker = GlobalMainFrame().getScopedScreenUpdateBlocker(_("Reloading Defs"),
        _("Reloading Defs"), true);
	
    reloadDefs();
}

// Gets called on VFS initialise
void EClassManager::onFileSystemInitialise()
{
	realise();
}

// Gets called on VFS shutdown
void EClassManager::onFileSystemShutdown()
{
	unrealise();
}

// Parse the provided stream containing the contents of a single .def file.
// Extract all entitydefs and create objects accordingly.
void EClassManager::parse(TextInputStream& inStr, const std::string& modDir)
{
	// Construct a tokeniser for the stream
	std::istream is(&inStr);
    parser::BasicDefTokeniser<std::istream> tokeniser(is);

    while (tokeniser.hasMoreTokens())
	{
        std::string blockType = tokeniser.nextToken();
        string::to_lower(blockType);

        if (blockType == "entitydef")
		{
			// Get the (lowercase) entity name
			const std::string sName =
    			string::to_lower_copy(tokeniser.nextToken());

			// Ensure that an Entity class with this name already exists
			// When reloading entityDef declarations, most names will already be registered
			EntityClasses::iterator i = _entityClasses.find(sName);

			if (i == _entityClasses.end())
			{
				// Not existing yet, allocate a new class
        		Doom3EntityClassPtr entityClass(new eclass::Doom3EntityClass(sName));

				std::pair<EntityClasses::iterator, bool> result = _entityClasses.insert(
					EntityClasses::value_type(sName, entityClass)
				);

				i = result.first;
			}
			else
			{
				// EntityDef already exists, compare the parse stamp
				if (i->second->getParseStamp() == _curParseStamp)
				{
					rWarning() << "[eclassmgr]: EntityDef "
						<< sName << " redefined" << std::endl;
				}
			}

			// At this point, i is pointing to a valid entityclass

			i->second->setParseStamp(_curParseStamp);

        	// Parse the contents of the eclass (excluding name)
			i->second->parseFromTokens(tokeniser);

			// Set the mod directory
        	i->second->setModName(modDir);
        }
        else if (blockType == "model")
		{
			// Read the name
			std::string modelDefName = tokeniser.nextToken();

			// Ensure that an Entity class with this name already exists
			// When reloading entityDef declarations, most names will already be registered
			Models::iterator i = _models.find(modelDefName);

			if (i == _models.end())
			{
				// Does not exist yet, allocate a new one

				// Allocate an empty ModelDef
        		Doom3ModelDefPtr model(new Doom3ModelDef(modelDefName));

				std::pair<Models::iterator, bool> result = _models.insert(
					Models::value_type(modelDefName, model)
				);

				i = result.first;
			}
			else
			{
				// Model already exists, compare the parse stamp
				if (i->second->getParseStamp() == _curParseStamp)
				{
					rWarning() << "[eclassmgr]: Model "
						<< modelDefName << " redefined" << std::endl;
				}
			}

			// Model structure is allocated and in the map,
            // invoke the parser routine
			i->second->setParseStamp(_curParseStamp);

        	i->second->parseFromTokens(tokeniser);
			i->second->setModName(modDir);
        }
    }
}

void EClassManager::parseFile(const std::string& filename)
{
	const std::string fullname = "def/" + filename;

	ArchiveTextFilePtr file = GlobalFileSystem().openTextFile(fullname);

	if (!file) return;

	try
    {
		// Parse entity defs from the file
		parse(file->getInputStream(), file->getModName());
	}
    catch (parser::ParseException& e)
    {
		rError() << "[eclassmgr] failed to parse " << filename
				 << " (" << e.what() << ")" << std::endl;
	}
}

// Static module instance
module::StaticModule<EClassManager> eclassModule;

} // namespace eclass
