#include "MenuManager.h"

#include "itextstream.h"
#include "iregistry.h"
#include "ui/imainframe.h"

#include "MenuBar.h"
#include "MenuFolder.h"
#include "MenuRootElement.h"
#include "module/StaticModule.h"

namespace ui
{

namespace menu
{

namespace
{
	// The menu root key in the registry
	const char* const RKEY_MENU_ROOT = "user/ui/menu";
}

MenuManager::MenuManager() :
	_root(new MenuRootElement())
{}

void MenuManager::clear()
{
	_root.reset();
}

void MenuManager::loadFromRegistry()
{
	// Clear existing elements first
	_root.reset(new MenuRootElement());

	xml::NodeList menuNodes = GlobalRegistry().findXPath(RKEY_MENU_ROOT);

	if (!menuNodes.empty())
	{
		for (const xml::Node& menuNode : menuNodes)
		{
			MenuElementPtr menubar = MenuElement::CreateFromNode(menuNode);

			// Add the menubar as child of the root
			_root->addChild(menubar);
		}
	}
	else
	{
		rError() << "MenuManager: Could not find menu root in registry." << std::endl;
	}
}

void MenuManager::setVisibility(const std::string& path, bool visible)
{
	// Sanity check for empty menu
	if (!_root) return;

	MenuElementPtr element = _root->find(path);

	if (element)
	{
		element->setIsVisible(visible);

		// The corresponding top level menu needs reconstruction
		MenuElementPtr parentMenu = findTopLevelMenu(element);

		if (parentMenu)
		{
			parentMenu->setNeedsRefresh(true);
		}
	}
}

wxMenuBar* MenuManager::getMenuBar(const std::string& name)
{
	if (!_root) return nullptr; // root has already been removed

	MenuElementPtr menuBar = _root->find(name);

	if (menuBar)
	{
		assert(std::dynamic_pointer_cast<MenuBar>(menuBar));

		return std::static_pointer_cast<MenuBar>(menuBar)->getMenuBar();
	}

	rError() << "MenuManager: Warning: Menubar with name " << name << " not found!" << std::endl;
	return nullptr;
}

void MenuManager::add(const std::string& insertPath,
							const std::string& name,
							ItemType type,
							const std::string& caption,
							const std::string& icon,
							const std::string& eventName)
{
	if (!_root) return; // root has already been removed

	MenuElementPtr parent = _root->find(insertPath);

	if (!parent)
	{
		rWarning() << "Cannot insert element at non-existent parent " << insertPath << std::endl;
		return;
	}

	MenuElementPtr element = MenuElement::CreateForType(type);

	element->setName(name);
	element->setCaption(caption);
	element->setIcon(icon);
	element->setEvent(eventName);

	parent->addChild(element);

	handleElementAdded(element);
}

void MenuManager::insert(const std::string& insertPath,
						 const std::string& name,
						 ItemType type,
						 const std::string& caption,
						 const std::string& icon,
						 const std::string& eventName)
{
	if (!_root) return; // root has already been removed

	MenuElementPtr insertBefore = _root->find(insertPath);

	if (!insertBefore || !insertBefore->getParent())
	{
		rWarning() << "Cannot insert before non-existent item or item doesn't have a parent"
			<< insertPath << std::endl;
		return;
	}

	MenuElementPtr element = MenuElement::CreateForType(type);

	element->setName(name);
	element->setCaption(caption);
	element->setIcon(icon);
	element->setEvent(eventName);

	insertBefore->getParent()->insertChild(element, insertBefore);

	handleElementAdded(element);
}

bool MenuManager::exists(const std::string& path)
{
	if (!_root) return false; // root has already been removed

	return _root->find(path) != nullptr;
}

void MenuManager::remove(const std::string& path)
{
	if (!_root) return; // root has already been removed

	MenuElementPtr element = _root->find(path);

	if (!element)
	{
		return; // no warning, some code just tries to remove stuff unconditionally
	}

	if (!element->getParent())
	{
		rWarning() << "Cannot remove item without a parent " << path << std::endl;
		return;
	}

	element->getParent()->removeChild(element);

	handleElementRemoved(element);
}

void MenuManager::handleElementAdded(const MenuElementPtr& element)
{
	// The corresponding top level menu needs reconstruction
	MenuElementPtr parentMenu = findTopLevelMenu(element);

	if (parentMenu)
	{
		parentMenu->setNeedsRefresh(true);
	}

#ifdef __linux__
	// In Linux, we don't receive the wxEVT_MENU_OPEN event on the MenuFolder
	// Option 1 is to add the event handler to the wxFrame housing the wxMenuBar
	//          but we don't always know what wxFrame that is (it's added later)
	// Option 2 is to mark the whole menu as "needs refresh" => let's go with this
    while (parentMenu)
    {
    	if (std::dynamic_pointer_cast<MenuBar>(parentMenu))
    	{
    		std::static_pointer_cast<MenuBar>(parentMenu)->setNeedsRefresh(true);
    		break;
    	}

    	parentMenu = parentMenu->getParent();
    }
#endif

	// When inserting a new menu in a menubar, make sure it is constructed
	if (element->getParent() &&
		std::dynamic_pointer_cast<MenuBar>(element->getParent()) &&
		std::static_pointer_cast<MenuBar>(element->getParent())->isConstructed())
	{
		std::static_pointer_cast<MenuBar>(element->getParent())->setNeedsRefresh(true);
	}
}

void MenuManager::handleElementRemoved(const MenuElementPtr& element)
{
	// The corresponding top level menu needs reconstruction
	MenuElementPtr parentMenu = findTopLevelMenu(element);

	if (parentMenu)
	{
		parentMenu->setNeedsRefresh(true);
	}
}

MenuElementPtr MenuManager::findTopLevelMenu(const MenuElementPtr& element)
{
	MenuElementPtr candidate = element;

	while (candidate)
	{
		MenuElementPtr parent = candidate->getParent();

		if (candidate &&
			std::dynamic_pointer_cast<MenuFolder>(candidate) &&
			std::dynamic_pointer_cast<MenuBar>(parent))
		{
			return candidate;
		}

		candidate = parent;
	}

	return MenuElementPtr();
}

const std::string& MenuManager::getName() const
{
    static std::string _name(MODULE_MENUMANAGER);
    return _name;
}

const StringSet& MenuManager::getDependencies() const
{
    static StringSet _dependencies
    {
        MODULE_MAINFRAME
    };

    return _dependencies;
}

void MenuManager::initialiseModule(const IApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;

    loadFromRegistry();

    GlobalMainFrame().signal_MainFrameShuttingDown().connect(
        sigc::mem_fun(this, &MenuManager::clear)
    );
}

void MenuManager::shutdownModule()
{
    clear();
}

module::StaticModuleRegistration<MenuManager> menuManagerModule;

} // namespace ui

}
