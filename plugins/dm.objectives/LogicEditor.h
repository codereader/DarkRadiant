#ifndef LOGIC_EDITOR_H_
#define LOGIC_EDITOR_H_

#include <string>
#include <gtkmm/table.h>

namespace Gtk
{
	class Entry;
}

namespace objectives
{

/**
 * greebo: This complex widget represents the UI elements needed to edit
 * a set of success- and failure logic strings.
 */
class LogicEditor :
	public Gtk::Table
{
private:
	Gtk::Entry* _successLogic;
	Gtk::Entry* _failureLogic;

public:
	/**
	 * The constructor will create the widgets.
	 */
	LogicEditor();

	// Read accessors for the logic strings
	std::string getSuccessLogicStr();
	std::string getFailureLogicStr();

	// Write accessors for the logic strings
	void setSuccessLogicStr(const std::string& logicStr);
	void setFailureLogicStr(const std::string& logicStr);
};

} // namespace objectives

#endif /* LOGIC_EDITOR_H_ */
