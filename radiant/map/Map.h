#ifndef MAPCLASS_H_
#define MAPCLASS_H_

#include "inode.h"
#include "imap.h"
#include "inamespace.h"
#include "imapresource.h"
#include "moduleobserver.h"
#include "math/Vector3.h"
#include "signal/signal.h"

class TextInputStream;

namespace map {

class Map : 
	public IMap,
	public IMapResource::Observer
{
	// The map name
	std::string m_name;

	// The name of the last "save copy as" filename
	std::string _lastCopyMapName;

	// Pointer to the resource for this map
	IMapResourcePtr m_resource;
	
	bool m_valid;

	bool m_modified;

	Signal0 m_mapValidCallbacks;

	scene::INodePtr m_world_node; // "classname" "worldspawn" !

	bool _saveInProgress;

public:
	Map();

	virtual scene::INodePtr getWorldspawn();
	virtual IMapRootNodePtr getRoot();

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);

	void realiseResource();
	void unrealiseResource();
	
	/** greebo: Returns true if the map has not been named yet.
	 */
	bool isUnnamed() const;
	
	/** greebo: Updates the name of the map (and triggers an update
	 * 			of the mainframe window title)
	 */
	void setMapName(const std::string& newName);
	
	/** greebo: Returns the name of this class
	 */
	std::string getMapName() const;
	
	/** 
	 * greebo: Saves the current map, doesn't ask for any filenames, 
	 * so this has to be done before this step.
	 *
	 * @returns: TRUE if the save was successful, FALSE if an error occurred.
	 */
	bool save();
	
	/** 
	 * greebo: Asks the user for a new filename and saves the map if 
	 * a valid filename was specified.
	 * 
	 * @returns: TRUE, if the user entered a valid filename and the map was 
	 * saved correctly. Returns FALSE if no valid filename was entered or 
	 * an error occurred.
	 */
	bool saveAs();

	/** 
	 * greebo: Saves a copy of the current map (asks for filename using 
	 * a dialog window).
	 */
	bool saveCopyAs();
	
	/** greebo: Saves the current selection to the target <filename>.
	 * 
	 * @returns: true on success.
	 */
	bool saveSelected(const std::string& filename);
	
	/** greebo: Loads the map from the given filename
	 */
	void load(const std::string& filename);
	
	/** greebo: Imports the contents from the given filename.
	 * 
	 * @returns: true on success.
	 */
	bool import(const std::string& filename);
	
	/** greebo: Exports the current map directly to the given filename.
	 * 			This skips any "modified" or "unnamed" checks, it just dumps
	 * 			the current scenegraph content to the file.
	 *
	 * @returns: true on success, false on failure.
	 */
	bool saveDirect(const std::string& filename);
	
	/** greebo: Creates a new map file.
	 * 
	 * Note: Can't be called "new" as this is a reserved word...
	 */
	void createNew();
	
	void rename(const std::string& filename);
	
	void importSelected(TextInputStream& in);
	void exportSelected(std::ostream& out);
	
	// free all map elements, reinitialize the structures that depend on them
	void freeMap();
	
	// Resource::Observer implementation
	void onResourceRealise();
	void onResourceUnrealise();
  
	// Accessor methods for the "valid" flag
	void setValid(bool valid);
	bool isValid() const;
	
	/** greebo: Returns true if the map has unsaved changes.
	 */
	bool isModified() const;
	
	// Sets the modified status of this map
	void setModified(bool modifiedFlag);
	
	SignalHandlerId addValidCallback(const SignalHandler& handler);
	void removeValidCallback(SignalHandlerId id);
	
	// Updates the window title of the mainframe
	void updateTitle();
	
	// Accessor methods for the worldspawn node
	void setWorldspawn(scene::INodePtr node);
	
	/** greebo: This retrieves the worldspawn node of this map.
	 *			If no worldspawn can be found, this creates one.
	 */
	scene::INodePtr findOrInsertWorldspawn();

	/** greebo: Tries to locate the worldspawn in the global scenegraph
	 *			Returns NULL (empty shared_ptr) if nothing is found. 
	 */
	scene::INodePtr findWorldspawn();
	
	/** greebo: Returns the map format for this map
	 */
	const MapFormat& getFormat();
	
	/** greebo: Returns the map format for the given filename
	 */
	static const MapFormat& getFormatForFile(const std::string& filename);
	
	/** greebo: Removes or saves the camera position (into worldspawn)
	 */
	void removeCameraPosition();
	void saveCameraPosition();
	
	/** greebo: Sets the camera to the start position. This uses
	 * 			the information stored in the worlspawn or
	 * 			the location of the info_player_start entity.
	 * 			If neither of these two exist, 0,0,0 is used. 
	 */
	void gotoStartPosition();
	
	/** greebo: Asks the user if the current changes should be saved.
	 * 
	 * @returns: true, if the user gave clearance (map was saved, had no
	 * 			 changes to be saved, etc.), false, if the user hit "cancel".
	 */
	bool askForSave(const std::string& title);

	/** greebo: Loads a prefab and translates it to the given target coordinates
	 */
	void loadPrefabAt(const Vector3& targetCoords);

	/** greebo: Focus the XYViews and the Camera to the given point/angle.
	 */
	static void focusViews(const Vector3& point, const Vector3& angles);

	/** greebo: Registers the commands with the EventManager.
	 */
	static void registerCommands();
	
	// Static command targets for connection to the EventManager
	static void exportMap();
	static void newMap();
	static void openMap();
	static void importMap();
	static void saveMap();
	static void saveMapAs();

	/** greebo: Queries a filename from the user and saves a copy 
	 *          of the current map to the specified filename.
	 */
	static void saveMapCopyAs();
	
	/** greebo: Asks the user for the .pfb file and imports/exports the file/selection
	 */
	static void loadPrefab();
	static void saveSelectedAsPrefab(); 

private:
	// If no worldspawn can be found in the scenegraph, this creates one
	void updateWorldspawn();

}; // class Map

} // namespace map

// Accessor function for the map
map::Map& GlobalMap();

#endif /*MAPCLASS_H_*/
