#include "PluginSlots.h"

#include "math/Vector3.h"
#include "string/string.h" // for the StringTokeniser stuff
#include "select.h"

// ============== CPlugInSlot Implementation ==========================================

// Constructor
CPluginSlot::CPluginSlot(GtkWidget* main_window, const std::string& name, const _QERPluginTable& table) {
  mpTable = &table;
  m_menu_name = name;

  const std::string commands = mpTable->m_pfnQERPlug_GetCommandList();
  const std::string titles = mpTable->m_pfnQERPlug_GetCommandTitleList();

  StringTokeniser commandTokeniser(commands.c_str(), ",;");
  StringTokeniser titleTokeniser(titles.c_str(), ",;");
  
  std::string cmdToken = commandTokeniser.getToken();
  std::string titleToken = titleTokeniser.getToken();
  while (cmdToken != "") 
  {
    if (titleToken == "") {
      m_CommandStrings.push_back(cmdToken);
      m_CommandTitleStrings.push_back(cmdToken);
      cmdToken = commandTokeniser.getToken();
      titleToken = "";
    }
    else {
      m_CommandStrings.push_back(cmdToken);
      m_CommandTitleStrings.push_back(titleToken);
      cmdToken = commandTokeniser.getToken();
      titleToken = titleTokeniser.getToken();
    }
  }
  mpTable->m_pfnQERPlug_Init(0, (void*)main_window);
}

const char* CPluginSlot::getMenuName() {
  return m_menu_name.c_str();
}

std::size_t CPluginSlot::getCommandCount() {
  return m_CommandStrings.size();  
}
  
const char* CPluginSlot::getCommand(std::size_t n) {
  std::list<std::string>::iterator i = m_CommandStrings.begin();
  while(n-- != 0)
    ++i;
  return (*i).c_str();  
}

const char* CPluginSlot::getCommandTitle(std::size_t n) {
  std::list<std::string>::iterator i = m_CommandTitleStrings.begin();
  while(n-- != 0)
    ++i;
  return (*i).c_str();  
}

void CPluginSlot::addMenuID(std::size_t n) {
  m_CommandIDs.push_back(n);
}

bool CPluginSlot::ownsCommandID(std::size_t n) {
  for(std::list<std::size_t>::iterator i = m_CommandIDs.begin(); i != m_CommandIDs.end(); ++i)
  {
    if (*i == n)
      return true;
  }
  return false;
}

void CPluginSlot::Dispatch(const std::string& p) {
  Vector3 vMin, vMax;
  Select_GetBounds (vMin, vMax);
  mpTable->m_pfnQERPlug_Dispatch(p.c_str(), reinterpret_cast<float*>(&vMin), reinterpret_cast<float*>(&vMax), true);//QE_SingleBrush(true));
}

// ============== CPlugInSlots Implementation ==========================================

// Destructor
CPluginSlots::~CPluginSlots()
{
  std::list<CPluginSlot *>::iterator iSlot;
  for(iSlot=mSlots.begin(); iSlot!=mSlots.end(); ++iSlot)
  {
    delete *iSlot;
    *iSlot = 0;
  }
}

void CPluginSlots::PopulateMenu(PluginsVisitor& menu)
{
	// Cycle through all the plugins and pass them to the PluginsVisitor class
  	std::list<CPluginSlot *>::iterator iPlug;
  	for (iPlug = mSlots.begin(); iPlug != mSlots.end(); ++iPlug) {
		menu.visit(*(*iPlug));
	}
}

// Dispatches a command to all the plugins  
bool CPluginSlots::Dispatch(std::size_t n, const std::string& p)
{
  std::list<CPluginSlot *>::iterator iPlug;
  for(iPlug=mSlots.begin(); iPlug!=mSlots.end(); ++iPlug)
  {
    CPluginSlot *pPlug = *iPlug;
    if (pPlug->ownsCommandID(n))
    {
      pPlug->Dispatch(p);
      return true;
    }
  }
  return false;
}
