#include "ToolbarManager.h"

#include <stdexcept>
#include "itextstream.h"
#include "ui/ieventmanager.h"
#include "iregistry.h"
#include "i18n.h"

#include <wx/toolbar.h>
#include "module/StaticModule.h"
#include "wxutil/Bitmap.h"

namespace ui
{

namespace
{
	const char* const CMD_VARIANT_NAME = "command";
}

const std::string& ToolbarManager::getName() const
{
    static std::string _name(MODULE_TOOLBARMANAGER);
    return _name;
}

const StringSet& ToolbarManager::getDependencies() const
{
    static StringSet _dependencies;

    if (_dependencies.empty())
    {
        _dependencies.insert(MODULE_XMLREGISTRY);
    }

    return _dependencies;
}

void ToolbarManager::initialiseModule(const IApplicationContext& ctx)
{
    _nextToolItemId = 100;

    try
    {
        // Query the registry
        loadToolbars();
    }
    catch (const std::runtime_error& e)
    {
        rError() << "ToolbarManager: Warning: " << e.what() << std::endl;
    }
}

wxToolBar* ToolbarManager::createToolbar(const std::string& toolbarName, wxWindow* parent)
{
	// Check if the toolbarName exists
    if (!toolbarExists(toolbarName))
    {
        rError() << "ToolbarManager: Critical: Named toolbar doesn't exist: " << toolbarName << std::endl;
	    return nullptr;
    }

	// Instantiate the toolbar with buttons
	rMessage() << "ToolbarManager: Instantiating toolbar: " << toolbarName << std::endl;

	// Build the path into the registry, where the toolbar should be found
	auto toolbarPath = std::string("//ui//toolbar") + "[@name='"+ toolbarName +"']";
	auto toolbarList = GlobalRegistry().findXPath(toolbarPath);

	if (toolbarList.empty())
	{
        rError() << "ToolbarManager: Critical: Could not instantiate " << toolbarName << std::endl;
	    return nullptr;
	}

	return createToolbarFromNode(toolbarList[0], parent);
}

wxToolBarToolBase* ToolbarManager::createToolItem(wxToolBar* toolbar, const xml::Node& node)
{
	const std::string nodeName = node.getName();

	wxToolBarToolBase* toolItem = nullptr;

	if (nodeName == "separator")
	{
		toolItem = toolbar->AddSeparator();
	}
	else if (nodeName == "toolbutton" || nodeName == "toggletoolbutton")
	{
		// Found a button, load the values that are shared by both types
		std::string name 		= node.getAttributeValue("name");
		std::string icon 		= node.getAttributeValue("icon");
		std::string tooltip 	= _(node.getAttributeValue("tooltip").c_str());
		std::string action 	= node.getAttributeValue("action");

        // Don't assign a label to the tool item since OSX is painting
        // the first few characters over the bitmap
        if (!icon.empty())
        {
            name.clear();
        }

		if (nodeName == "toolbutton")
		{
			// Create a new ToolButton and assign the right callback
			toolItem = toolbar->AddTool(_nextToolItemId++, name,
				wxutil::GetLocalBitmap(icon),
				tooltip);
		}
		else
		{
			// Create a new ToggleToolButton and assign the right callback
			toolItem = toolbar->AddTool(_nextToolItemId++, name,
				wxutil::GetLocalBitmap(icon),
				tooltip, wxITEM_CHECK);
		}

		toolItem->SetClientData(new wxVariant(action, CMD_VARIANT_NAME));

		GlobalEventManager().registerToolItem(action, toolItem);
	}

	return toolItem;
}

wxToolBar* ToolbarManager::createToolbarFromNode(xml::Node& node, wxWindow* parent)
{
	// Get all action children elements
	auto toolItemList = node.getChildren();
	wxToolBar* toolbar = nullptr;

    if (toolItemList.empty())
    {
        throw std::runtime_error("No elements in toolbar.");
    }

	// Try to set the alignment, if the attribute is properly set
	auto align = node.getAttributeValue("align");

	// Create a new toolbar
	toolbar = new wxToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		align == "vertical" ? wxTB_VERTICAL : wxTB_HORIZONTAL,
		node.getAttributeValue("name"));

    // Adjust the toolbar bitmap size to add some padding - despite its name
    // this will not resize the actual icons, just the buttons
    toolbar->SetToolBitmapSize(wxSize(20, 20));

	for (const auto& toolNode : toolItemList)
	{
		// Create and get the toolItem with the parsing
		createToolItem(toolbar, toolNode);
	}

	toolbar->Realize();
	toolbar->Bind(wxEVT_DESTROY, &ToolbarManager::onToolbarDestroy, this);

	return toolbar;
}

void ToolbarManager::onToolbarDestroy(wxWindowDestroyEvent& ev)
{
	auto toolbar = wxDynamicCast(ev.GetEventObject(), wxToolBar);

	if (toolbar == nullptr)
	{
		return;
	}

	for (std::size_t tool = 0; tool < toolbar->GetToolsCount(); tool++)
	{
		auto toolItem = toolbar->GetToolByPos(tool);

		auto cmdData = wxDynamicCast(toolItem->GetClientData(), wxVariant);

		if (cmdData != nullptr && cmdData->GetName() == CMD_VARIANT_NAME)
		{
			GlobalEventManager().unregisterToolItem(cmdData->GetString().ToStdString(), toolItem);

			// free the client data, the toolbar item doesn't delete it
			toolbar->SetToolClientData(toolItem->GetId(), nullptr);
			delete cmdData;
		}
	}
}

bool ToolbarManager::toolbarExists(const std::string& toolbarName)
{
	return _toolbars.count(toolbarName) > 0;
}

void ToolbarManager::loadToolbars()
{
	auto toolbarList = GlobalRegistry().findXPath("//ui//toolbar");

    if (toolbarList.empty())
    {
		throw std::runtime_error("No toolbars found.");
    }

	for (std::size_t i = 0; i < toolbarList.size(); ++i)
	{
		auto toolbarName = toolbarList[i].getAttributeValue("name");

		if (toolbarExists(toolbarName))
		{
			//rMessage() << "This toolbar already exists: ";
			continue;
		}

		rMessage() << "Found toolbar: " << toolbarName << std::endl;

		_toolbars.insert(toolbarName);
	}
}

module::StaticModuleRegistration<ToolbarManager> toolbarManagerModule;

} // namespace ui
