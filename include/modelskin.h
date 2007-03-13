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

#if !defined(INCLUDED_MODELSKIN_H)
#define INCLUDED_MODELSKIN_H

#include "generic/constant.h"

#include <vector>
#include <string>

class ModuleObserver;

class ModelSkin
{
public:
	STRING_CONSTANT(Name, "ModelSkin");

	/// \brief Attach an \p observer whose realise() and unrealise() methods will be called when the skin is loaded or unloaded.
	virtual void attach(ModuleObserver& observer) = 0;
	
	/// \brief Detach an \p observer previously-attached by calling \c attach.
	virtual void detach(ModuleObserver& observer) = 0;

	/**
	 * Get the mapped texture for the given query texture, using the mappings
	 * in this skin. If there is no mapping for the given texture, return an
	 * empty string.
	 */
	virtual std::string getRemap(const std::string& name) const = 0;
};

class SkinnedModel
{
public:
  STRING_CONSTANT(Name, "SkinnedModel");
  /// \brief Instructs the skinned model to update its skin.
  virtual void skinChanged() = 0;
};

// Model skinlist typedef
typedef std::vector<std::string> StringList;

/**
 * Interface class for the skin manager.
 */
struct ModelSkinCache
{
	INTEGER_CONSTANT(Version, 1);
	STRING_CONSTANT(Name, "modelskin");

	/**
	 * Lookup a specific named skin and return the corresponding ModelSkin
	 * object.
	 */
	virtual ModelSkin& capture(const std::string& name) = 0;
	
	/** 
	 * Return the skins associated with the given model.
	 * 
	 * @param
	 * The full pathname of the model, as given by the "model" key in the skin 
	 * definition.
	 * 
	 * @returns
	 * A vector of strings, each identifying the name of a skin which is 
	 * associated with the given model. The vector may be empty as a model does
	 * not require any associated skins.
	 */
	virtual const StringList& getSkinsForModel(const std::string& model) = 0;
	
	/**
	 * Return the complete list of available skins.
	 */
	virtual const StringList& getAllSkins() = 0;
};


#include "modulesystem.h"

template<typename Type>
class GlobalModule;
typedef GlobalModule<ModelSkinCache> GlobalModelSkinCacheModule;

template<typename Type>
class GlobalModuleRef;
typedef GlobalModuleRef<ModelSkinCache> GlobalModelSkinCacheModuleRef;

inline ModelSkinCache& GlobalModelSkinCache()
{
  return GlobalModelSkinCacheModule::getTable();
}

#endif
