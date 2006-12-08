/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "entity.h"

#include "ieclass.h"
#include "ifilter.h"
#include "selectable.h"
#include "namespace.h"

#include "scenelib.h"
#include "entitylib.h"
#include "pivot.h"

#include "targetable.h"
#include "uniquenames.h"
#include "namekeys.h"
#include "stream/stringstream.h"

#include "light.h"
#include "eclassmodel.h"
#include "generic.h"
#include "doom3group.h"

#include <iostream>
#include <boost/algorithm/string/replace.hpp>

EGameType g_gameType;

scene::Node& entity_for_eclass(IEntityClass* eclass)
{
	if(eclass->isLight()) {
	    return New_Light(eclass);
	}
	else if(!eclass->isFixedSize()) {
		// Variable size entity
	    return New_Doom3Group(eclass);
	}
	else if(eclass->getModelPath().size() > 0) {
		// Fixed size, has model path
		return New_EclassModel(eclass);
	}
	else {
		// Fixed size, no model path
    	return New_GenericEntity(eclass);
	}
}

void Entity_setName(Entity& entity, const char* name)
{
  entity.setKeyValue("name", name);
}
typedef ReferenceCaller1<Entity, const char*, Entity_setName> EntitySetNameCaller;

inline Namespaced* Node_getNamespaced(scene::Node& node)
{
  return NodeTypeCast<Namespaced>::cast(node);
}

/* Cleans up the name of the entity that is about the created
 * so that nothing bad can happen (for example, the colon character 
 * seems to be causing problems in Doom 3 Scripting)
 */
std::string cleanEntityName(const std::string& rawName) {
	std::string returnValue = rawName;
	boost::algorithm::replace_all(returnValue, ":", "_");
	return returnValue;
}

scene::Node& node_for_eclass(IEntityClass* eclass)
{
    
  scene::Node& node = entity_for_eclass(eclass);
  Node_getEntity(node)->setKeyValue("classname", eclass->getName());

	// If this is not a worldspawn or unrecognised entity, generate a unique
	// name for it
	if(eclass->getName().size() > 0
       && eclass->getName() != "worldspawn"
       && eclass->getName() != "UNKNOWN_CLASS") 
	{
		std::string entityName = cleanEntityName(eclass->getName()) + "_1";
    	GlobalNamespace().makeUnique(entityName.c_str(), EntitySetNameCaller(*Node_getEntity(node)));
	}

  Namespaced* namespaced = Node_getNamespaced(node);
  if(namespaced != 0)
  {
    namespaced->setNamespace(GlobalNamespace());
  }

  return node;
}

EntityCreator::KeyValueChangedFunc EntityKeyValues::m_entityKeyValueChanged = 0;
EntityCreator::KeyValueChangedFunc KeyValue::m_entityKeyValueChanged = 0;
Counter* EntityKeyValues::m_counter = 0;

bool g_showNames = true;
bool g_showAngles = true;
bool g_newLightDraw = true;

class ConnectEntities
{
public:
  Entity* m_e1;
  Entity* m_e2;
  ConnectEntities(Entity* e1, Entity* e2) : m_e1(e1), m_e2(e2)
  {
  }
  void connect(const char* name)
  {
	  m_e1->setKeyValue("target", name);
	  m_e2->setKeyValue("targetname", name);
  }
  typedef MemberCaller1<ConnectEntities, const char*, &ConnectEntities::connect> ConnectCaller;
};

inline Entity* ScenePath_getEntity(const scene::Path& path)
{
  Entity* entity = Node_getEntity(path.top());
  if(entity == 0)
  {
    entity = Node_getEntity(path.parent());
  }
  return entity;
}

class Quake3EntityCreator : public EntityCreator
{
public:
  scene::Node& createEntity(IEntityClass* eclass)
  {
    return node_for_eclass(eclass);
  }
  void setKeyValueChangedFunc(KeyValueChangedFunc func)
  {
    EntityKeyValues::setKeyValueChangedFunc(func);
  }
  void setCounter(Counter* counter)
  {
    EntityKeyValues::setCounter(counter);
  }
  
	/* Connect two entities using a "target" key.
	 */
	void connectEntities(const scene::Path& path, 
						 const scene::Path& targetPath)
	{
		// Obtain both entities
		Entity* e1 = ScenePath_getEntity(path);
		Entity* e2 = ScenePath_getEntity(targetPath);

		// Check entities are valid
		if(e1 == 0 || e2 == 0) {
			globalErrorStream() << "entityConnectSelected: both of the selected instances must be an entity\n";
			return;
		}

		// Check entities are distinct
		if(e1 == e2) {
			globalErrorStream() << "entityConnectSelected: the selected instances must not both be from the same entity\n";
			return;
		}

		// Start the scoped undo session
		UndoableCommand undo("entityConnectSelected");

		// Find the first unused target key on the source entity
		for (int i = 0; i < 1024; ++i) {

			// Construct candidate key by appending number to "target"
			std::string targetKey = (boost::format("target%i") % i).str();
			
			// If the source entity does not have this key, add it and finish,
			// otherwise continue looping
			if (e1->getKeyValue(targetKey).empty()) {
				e1->setKeyValue(targetKey,
								e2->getKeyValue("name"));
				break;
			}
		}

		// Redraw the scene
		SceneChangeNotify();
	}

  void setShowNames(bool showNames)
  {
    g_showNames = showNames;
  }
  bool getShowNames()
  {
    return g_showNames;
  }
  void setShowAngles(bool showAngles)
  {
    g_showAngles = showAngles;
  }
  bool getShowAngles()
  {
    return g_showAngles;
  }

  void printStatistics() const
  {
    StringPool_analyse(EntityKeyValues::getPool());
  }
};

Quake3EntityCreator g_Quake3EntityCreator;

EntityCreator& GetEntityCreator()
{
  return g_Quake3EntityCreator;
}

#include "preferencesystem.h"

/* greebo: Constructs the entity environment according to the given game type (is Doom3 anyway)
 */
void Entity_Construct(EGameType gameType)
{
  g_gameType = gameType;
  g_targetable_nameKey = "name";

  Static<KeyIsName>::instance().m_keyIsName = keyIsNameDoom3;
  Static<KeyIsName>::instance().m_nameKey = "name";

	// Register the entity preferences
  GlobalPreferenceSystem().registerPreference("SI_ShowNames", BoolImportStringCaller(g_showNames), BoolExportStringCaller(g_showNames));
  GlobalPreferenceSystem().registerPreference("SI_ShowAngles", BoolImportStringCaller(g_showAngles), BoolExportStringCaller(g_showAngles));
  // greebo: disabled this, it doesn't seem to be used anymore
  //GlobalPreferenceSystem().registerPreference("NewLightStyle", BoolImportStringCaller(g_newLightDraw), BoolExportStringCaller(g_newLightDraw));

	// Setup the lights
  //LightType lightType = LIGHTTYPE_DOOM3;
  Light_Construct(LIGHTTYPE_DOOM3);
  Doom3Group_construct();

  RenderablePivot::StaticShader::instance() = GlobalShaderCache().capture("$PIVOT");

  GlobalShaderCache().attachRenderable(StaticRenderableConnectionLines::instance());
}

void Entity_Destroy()
{
  GlobalShaderCache().detachRenderable(StaticRenderableConnectionLines::instance());

  GlobalShaderCache().release("$PIVOT");

  Doom3Group_destroy();
  Light_Destroy();
}
