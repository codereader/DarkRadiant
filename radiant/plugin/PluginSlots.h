#ifndef PLUGINSLOTS_H_
#define PLUGINSLOTS_H_

/* greebo: A plugin is contained in the class CPluginSlot and multiple PluginSlots are
 * owned by the class CPluginSlots. 
 */
 
#include <list>
#include <string>
#include "iplugin.h"
#include "PluginsVisitor.h"

// Forward declaration to avoid including the whole gtk stuff
typedef struct _GtkWidget GtkWidget;

// =================================================================================

/* greebo: This is a PlugInSlot containing a single plugin. It maintains a list of 
 * commands that are necessary to integrate it into the radiant core and the
 * plugin interface. 
 */
class CPluginSlot : 
	public IPlugin
{
  std::string m_menu_name;
  const _QERPluginTable *mpTable;
  std::list<std::string> m_CommandStrings;
  std::list<std::string> m_CommandTitleStrings;
  std::list<std::size_t> m_CommandIDs;
  
public:
  /*!
  build directly from a SYN_PROVIDE interface
  */
  CPluginSlot(GtkWidget* main_window, const std::string& name, const _QERPluginTable& table);
  /*!
  dispatching a command by name to the plugin
  */
  void Dispatch(const std::string& p);

  // IPlugin ------------------------------------------------------------
  const char* getMenuName();
  std::size_t getCommandCount();
  const char* getCommand(std::size_t n);  
  const char* getCommandTitle(std::size_t n);
  void addMenuID(std::size_t n);
  bool ownsCommandID(std::size_t n);
};

// =================================================================================

class CPluginSlots
{
  std::list<CPluginSlot*> mSlots;
public:
  virtual ~CPluginSlots();

  void AddPluginSlot(GtkWidget* main_window, const std::string& name, const _QERPluginTable& table)
  {
    mSlots.push_back(new CPluginSlot(main_window, name, table));
  }
  
  void PopulateMenu(PluginsVisitor& menu);
  bool Dispatch(std::size_t n, const std::string& p);
};

#endif /*PLUGINSLOTS_H_*/
