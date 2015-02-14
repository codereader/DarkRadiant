#pragma once

#include <wx/progdlg.h>
#include <string>
#include <memory>

namespace wxutil
{

class ModalProgressDialog :
	public wxProgressDialog
{
public:
	/** 
	 * Constructor accepts window to be modal for and the dialog title.
	 */
	ModalProgressDialog(const std::string& title, wxWindow* parent = nullptr);

	/**
	 * Exception thrown when cancel button is pressed.
	 */
	struct OperationAbortedException
	: public std::runtime_error
	{
		OperationAbortedException(const std::string& what)
		: std::runtime_error(what) {}
	};

	/**
	 * Set the text to display in the label, and pulse the progress bar. If the
	 * user has clicked the Cancel button since the last update, this method
	 * will throw an exception to indicate an aborted operation.
	 */
	void setText(const std::string& text);

	/**
	 * Set the text to display in the label, and the completed fraction of the progress bar.
	 * If the user has clicked the Cancel button since the last update, this method
	 * will throw an exception to indicate an aborted operation.
	 */
	void setTextAndFraction(const std::string& text, double fraction);
};
typedef std::shared_ptr<ModalProgressDialog> ModalProgressDialogPtr;

} // namespace
