#ifndef ORTHOCONTEXTMENU_H_
#define ORTHOCONTEXTMENU_H_

#include "iorthocontextmenu.h"
#include <map>
#include <list>
#include "math/Vector3.h"

typedef struct _GtkMenuItem GtkMenuItem;
typedef struct _GtkWidget GtkWidget;

namespace ui
{

// Forward declaration
class LayerContextMenu;
typedef boost::shared_ptr<LayerContextMenu> LayerContextMenuPtr;

/** Displays a menu when the mouse is right-clicked in the ortho window.
 * This is a singleton module which remains in existence once initialised,
 * and is hidden and displayed as appropriate.
 */
class OrthoContextMenu :
	public IOrthoContextMenu
{
	// The GtkWidget representing the menu
	GtkWidget* _widget;
	
	// Last provided 3D point for action
	Vector3 _lastPoint;

	// The widgets, indexed by an enum
	std::map<int, GtkWidget*> _widgets;
	
	LayerContextMenuPtr _addToLayerSubmenu;
	LayerContextMenuPtr _moveToLayerSubmenu;
	LayerContextMenuPtr _removeFromLayerSubmenu;

	// A list of menu items
	typedef std::list<IOrthoContextMenuItemPtr> MenuItems;

	// The menu sections, distinguished by section number
	typedef std::map<int, MenuItems> MenuSections;
	MenuSections _sections;

private:

    static std::string getRegistryKeyWithDefault(const std::string&,
                                                 const std::string&);

	// Enable or disable the "convert to static" option based on the number
	// of selected brushes.
	void checkConvertStatic();
	
	/** greebo: Enable or disables the "revert to worldspawn" option 
	 */
	void checkRevertToWorldspawn();
	
	/** greebo: Disables the "entity/light/speaker/playerStart" options according to the selection,
	 *			and change "playerStart" if another playerStart is found.
	 */
	void checkAddOptions();

	// Disables the "add MonsterClip" option according to the selection,
	void checkMonsterClip();

	// mohij: changes the "Add PlayerStart" entry if an info_player_start already exists
	void checkPlayerStart();

	void checkMakeVisportal();

	// Refreshes the layer submenus
	void repopulateLayerMenus();

	/* Gtk Callbacks */
	
	static void callbackAddEntity(GtkMenuItem* item, OrthoContextMenu* self);
	static void callbackAddPlayerStart(GtkMenuItem* item, OrthoContextMenu* self);
	static void callbackMovePlayerStart(GtkMenuItem* item, OrthoContextMenu* self);
	static void callbackAddModel(GtkMenuItem* item, OrthoContextMenu* self);
	static void callbackAddMonsterClip(GtkMenuItem* item, OrthoContextMenu* self);
	static void callbackAddLight(GtkMenuItem* item, OrthoContextMenu* self);
	static void callbackAddPrefab(GtkMenuItem* item, OrthoContextMenu* self);
	static void callbackAddSpeaker(GtkMenuItem*, OrthoContextMenu* self);

	// Gets called by the items in the "Add to Layer" submenu
	static void callbackAddToLayer(int layerID);
	static void callbackMoveToLayer(int layerID);
	static void callbackRemoveFromLayer(int layerID);
	
public:
	OrthoContextMenu();

	/** Display the menu at the current mouse position, and act on the
	 * choice.
	 * 
	 * @param point
	 * The point in 3D space at which the chosen operation should take
	 * place.
	 */
	void showAt(const Vector3& point);
	
	// Retrieve the singleton instance
	static OrthoContextMenu& Instance();

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);

	// IOrthoContextMenu implementation
	void addItem(const IOrthoContextMenuItemPtr& item, int section);
	void removeItem(const IOrthoContextMenuItemPtr& item);

private:
	// Create, pack and connect widgets
	void constructMenu();

	void addSectionItems(int section);
};

}

#endif /*ORTHOCONTEXTMENU_H_*/
