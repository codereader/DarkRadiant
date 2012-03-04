#ifndef ORTHOCONTEXTMENU_H_
#define ORTHOCONTEXTMENU_H_

#include "iorthocontextmenu.h"
#include "iradiant.h"

#include <map>
#include <list>
#include "math/Vector3.h"

#include <boost/enable_shared_from_this.hpp>

namespace Gtk
{
	class Menu;
}

namespace ui
{

/** Displays a menu when the mouse is right-clicked in the ortho window.
 * This is a singleton module which remains in existence once initialised,
 * and is hidden and displayed as appropriate.
 */
class OrthoContextMenu :
	public IOrthoContextMenu,
	public boost::enable_shared_from_this<OrthoContextMenu>
{
	// The GtkWidget representing the menu
	boost::shared_ptr<Gtk::Menu> _widget;

	// Last provided 3D point for action
	Vector3 _lastPoint;

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
	void shutdownModule();

	// IOrthoContextMenu implementation
	void addItem(const IMenuItemPtr& item, int section);
	void removeItem(const IMenuItemPtr& item);

private:

    static std::string getRegistryKeyWithDefault(const std::string&,
                                                 const std::string&);

	void analyseSelection();

	// Visibility/Sensitivity checks
	bool checkConvertStatic();
	bool checkRevertToWorldspawn();
	bool checkMergeEntities();
	bool checkReparentPrimitives();
	bool checkRevertToWorldspawnPartial();
	bool checkAddPlayerStart();
	bool checkMovePlayerStart();
	bool checkMakeVisportal();
	bool checkAddMonsterclip();
	bool checkAddEntity();
	bool checkAddModel();

	void addEntity();
	void addPlayerStart();
	void callbackMovePlayerStart();
	void callbackAddModel();
	void callbackAddLight();
	void callbackAddPrefab();
	void callbackAddSpeaker();

	void registerDefaultItems();

	// Pack widgets
	void constructMenu();

	void addSectionItems(int section, bool noSpacer = false);
};

}

#endif /*ORTHOCONTEXTMENU_H_*/
