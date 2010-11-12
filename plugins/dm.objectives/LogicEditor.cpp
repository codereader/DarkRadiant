#include "LogicEditor.h"

#include "i18n.h"
#include "gtkutil/LeftAlignedLabel.h"

#include <gtkmm/entry.h>

namespace objectives {

LogicEditor::LogicEditor() :
	Gtk::Table(2, 2, false)
{
	// Create the text entry fields
	_successLogic = Gtk::manage(new Gtk::Entry);
	_failureLogic = Gtk::manage(new Gtk::Entry);

	// Create the labels for each text entry field
	Gtk::Label* successLogicLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_("Success Logic:")));
	Gtk::Label* failureLogicLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_("Failure Logic:")));

	// Pack the label and the widget into the table
	set_row_spacings(6);
	set_col_spacings(12);

	int row = 0;

	// pack the success logic
	attach(*successLogicLabel, 0, 1, row, row+1, Gtk::FILL, Gtk::FILL, 0, 0);
	attach(*_successLogic, 1, 2, row, row+1);

	row++;

	// pack the failure logic
	attach(*failureLogicLabel, 0, 1, row, row+1, Gtk::FILL, Gtk::FILL, 0, 0);
	attach(*_failureLogic, 1, 2, row, row+1);
}

std::string LogicEditor::getSuccessLogicStr()
{
	return _successLogic->get_text();
}

std::string LogicEditor::getFailureLogicStr()
{
	return _failureLogic->get_text();
}

void LogicEditor::setSuccessLogicStr(const std::string& logicStr)
{
	_successLogic->set_text(logicStr);
}

void LogicEditor::setFailureLogicStr(const std::string& logicStr)
{
	_failureLogic->set_text(logicStr);
}

} // namespace objectives
