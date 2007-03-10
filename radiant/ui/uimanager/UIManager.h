#ifndef UIMANAGER_H_
#define UIMANAGER_H_

#include "iuimanager.h"

#include <iostream>

namespace ui {

class UIManager :
	public IUIManager
{
public:
	void addMenuItem(const std::string& menuPath, const std::string& commandName) {

	}
}; // class UIManager

} // namespace ui

#endif /*UIMANAGER_H_*/
