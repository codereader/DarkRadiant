#ifndef ORTHOCONTEXTMENU_H_
#define ORTHOCONTEXTMENU_H_

#include "iorthocontextmenu.h"
#include "iradiant.h"
#include <map>
#include <list>
#include "math/Vector3.h"
#include <boost/enable_shared_from_this.hpp>

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
	public IOrthoContextMenu,
	public RadiantEventListener,
	public boost::enable_shared_from_this<OrthoContextMenu>
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
	typedef std::list<IMenuItemPtr> MenuItems;

	// The menu sections, distinguished by section number
	typedef std::map<int, MenuItems> MenuSections;
	MenuSections _sections;

	struct ExtendedSelectionInfo
	{
		bool anythingSelected;

		bool onlyPrimitivesSelected;
		bool onlyBrushesSelected;
		bool onlyPatchesSelected;
		bool singlePrimitiveSelected;

		bool onlyEntitiesSelected;

		bool onlyGroupsSelected;
		bool singleGroupSelected;

		bool onlyModelsSelected;

		bool playerStartExists;
	};

	ExtendedSelectionInfo _selectionInfo;

private:

    static std::string getRegistryKeyWithDefault(const std::string&,
                                                 const std::string&);

	void analyseSelection();

	// Enable or disable the "convert to static" option based on the number of selected brushes.
	bool checkConvertStatic();
	bool checkRevertToWorldspawn();
	bool checkMergeEntities();
	bool checkRevertToWorldspawnPartial();
	bool checkAddPlayerStart();
	bool checkMovePlayerStart();
	bool checkMakeVisportal();
	bool checkAddMonsterclip();
	bool checkAddEntity();
	bool checkAddModel();
	
	void callbackAddEntity();
	void callbackAddPlayerStart();
	void callbackMovePlayerStart();
	void callbackAddModel();
	void callbackAddLight();
	void callbackAddPrefab();
	void callbackAddSpeaker();
	
public:
	OrthoContextMenu();

	/** Display the menu at the current mouse position, and act on the
	 * choice.
	 * 
	 * @param point
	 * The point in 3D space at which the chosen operation should take
	 * place.
	 */
	void show(const Vector3& point);
	
	// Retrieve the singleton instance
	static OrthoContextMenu& Instance();

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);

	// IOrthoContextMenu implementation
	void addItem(const IMenuItemPtr& item, int section);
	void removeItem(const IMenuItemPtr& item);

	void onRadiantStartup();

private:
	// Create, pack and connect widgets
	void constructMenu();

	void addSectionItems(int section);

	// Private add method, to ensure that items are added at the front
	void addItemFront(const IMenuItemPtr& item, int section);
	void addItem(const IMenuItemPtr& item, int section, bool atFront);
};

}

#endif /*ORTHOCONTEXTMENU_H_*/
