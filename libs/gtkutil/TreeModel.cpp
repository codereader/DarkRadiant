#include "TreeModel.h"

#include <gtkmm/treeview.h>
#include <boost/algorithm/string/find.hpp>

namespace gtkutil
{

bool TreeModel::equalFuncStringContains(const Glib::RefPtr<Gtk::TreeModel>& model,
										int column,
										const Glib::ustring& key,
										const Gtk::TreeModel::iterator& iter)
{
	// Retrieve the string from the model
	std::string str;
	iter->get_value(column, str);

	// Use a case-insensitive search
	boost::iterator_range<std::string::iterator> range = boost::algorithm::ifind_first(str, key);

	// Returning FALSE means "match".
	return (!range.empty()) ? false : true;
}

TreeModel::SelectionFinder::SelectionFinder(const std::string& selection, int column) :
	_selection(selection),
	_needle(0),
	_column(column),
	_searchForInt(false)
{}

TreeModel::SelectionFinder::SelectionFinder(int needle, int column) :
	_selection(""),
	_needle(needle),
	_column(column),
	_searchForInt(true)
{}

const Gtk::TreeModel::iterator TreeModel::SelectionFinder::getIter() const
{
	return _foundIter;
}

bool TreeModel::SelectionFinder::forEach(const Gtk::TreeModel::iterator& iter)
{
	// If the visited row matches the string/int to find
	if (_searchForInt)
	{
		int value;
		iter->get_value(_column, value);

		if (value == _needle)
		{
			_foundIter = iter;
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		// Search for string
		Glib::ustring value;
		iter->get_value(_column, value);

		if (value == _selection)
		{
			_foundIter = iter;
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool TreeModel::findAndSelectString(Gtk::TreeView* view, const std::string& needle, int column)
{
	SelectionFinder finder(needle, column);

	view->get_model()->foreach_iter(sigc::mem_fun(finder, &SelectionFinder::forEach));

	if (finder.getIter())
	{
		Gtk::TreeModel::Path path(finder.getIter());

		// Expand the treeview to display the target row
		view->expand_to_path(path);
		// Highlight the target row
		view->set_cursor(path);
		// Make the selected row visible
		view->scroll_to_row(path, 0.3f);

		return true; // found
	}

	return false; // not found
}

bool TreeModel::findAndSelectInteger(Gtk::TreeView* view, int needle, int column)
{
	SelectionFinder finder(needle, column);

	view->get_model()->foreach_iter(sigc::mem_fun(finder, &SelectionFinder::forEach));

	if (finder.getIter())
	{
		Gtk::TreeModel::Path path(finder.getIter());

		// Expand the treeview to display the target row
		view->expand_to_path(path);
		// Highlight the target row
		view->set_cursor(path);
		// Make the selected row visible
		view->scroll_to_row(path, 0.3f);

		return true; // found
	}

	return false; // not found
}

bool TreeModel::findAndSelectString(Gtk::TreeView* view, const std::string& needle,
									const Gtk::TreeModelColumn<Glib::ustring>& column)
{
	return findAndSelectString(view, needle, column.index());
}

bool TreeModel::findAndSelectString(Gtk::TreeView* view, const std::string& needle,
									const Gtk::TreeModelColumn<std::string>& column)
{
	return findAndSelectString(view, needle, column.index());
}

bool TreeModel::findAndSelectInteger(Gtk::TreeView* view, int needle,
									const Gtk::TreeModelColumn<int>& column)
{
	return findAndSelectInteger(view, needle, column.index());
}

int TreeModel::sortFuncFoldersFirstStd(const Gtk::TreeModel::iterator& a,
									const Gtk::TreeModel::iterator& b,
									const Gtk::TreeModelColumn<std::string>& nameColumn,
									const Gtk::TreeModelColumn<bool>& isFolderColumn)
{
	Gtk::TreeModel::Row rowA = *a;
	Gtk::TreeModel::Row rowB = *b;

	// Check if A or B are folders
	bool aIsFolder = rowA[isFolderColumn];
	bool bIsFolder = rowB[isFolderColumn];

	if (aIsFolder)
	{
		// A is a folder, check if B is as well
		if (bIsFolder)
		{
			// A and B are both folders, compare names
			// greebo: We're not checking for equality here, names should be unique
			return std::string(rowA[nameColumn]) < std::string(rowB[nameColumn]) ? -1 : 1;
		}
		else
		{
			// A is a folder, B is not, A sorts before
			return -1;
		}
	}
	else
	{
		// A is not a folder, check if B is one
		if (bIsFolder)
		{
			// A is not a folder, B is, so B sorts before A
			return 1;
		}
		else
		{
			// Neither A nor B are folders, compare names
			// greebo: We're not checking for equality here, names should be unique
			return std::string(rowA[nameColumn]) < std::string(rowB[nameColumn]) ? -1 : 1;
		}
	}
}

void TreeModel::applyFoldersFirstSortFunc(const Glib::RefPtr<Gtk::TreeSortable>& model,
										  const Gtk::TreeModelColumn<std::string>& nameColumn,
										  const Gtk::TreeModelColumn<bool>& isFolderColumn)
{
	// Set the sort column ID to nameCol
	model->set_sort_column(nameColumn, Gtk::SORT_ASCENDING);

	// Then apply the custom sort function, bind the folder column to the functor
	model->set_sort_func(nameColumn, sigc::bind(sigc::ptr_fun(sortFuncFoldersFirstStd), nameColumn, isFolderColumn));
}

int TreeModel::sortFuncFoldersFirst(const Gtk::TreeModel::iterator& a,
									  const Gtk::TreeModel::iterator& b,
									  const Gtk::TreeModelColumn<Glib::ustring>& nameColumn,
									  const Gtk::TreeModelColumn<bool>& isFolderColumn)
{
	Gtk::TreeModel::Row rowA = *a;
	Gtk::TreeModel::Row rowB = *b;

	// Check if A or B are folders
	bool aIsFolder = rowA[isFolderColumn];
	bool bIsFolder = rowB[isFolderColumn];

	if (aIsFolder)
	{
		// A is a folder, check if B is as well
		if (bIsFolder)
		{
			// A and B are both folders, compare names
			// greebo: We're not checking for equality here, names should be unique
			return (Glib::ustring(rowA[nameColumn]) < Glib::ustring(rowB[nameColumn])) ? -1 : 1;
		}
		else
		{
			// A is a folder, B is not, A sorts before
			return -1;
		}
	}
	else
	{
		// A is not a folder, check if B is one
		if (bIsFolder)
		{
			// A is not a folder, B is, so B sorts before A
			return 1;
		}
		else
		{
			// Neither A nor B are folders, compare names
			// greebo: We're not checking for equality here, names should be unique
			return (Glib::ustring(rowA[nameColumn]) < Glib::ustring(rowB[nameColumn])) ? -1 : 1;
		}
	}
}

void TreeModel::applyFoldersFirstSortFunc(const Glib::RefPtr<Gtk::TreeSortable>& model,
										  const Gtk::TreeModelColumn<Glib::ustring>& nameColumn,
										  const Gtk::TreeModelColumn<bool>& isFolderColumn)
{
	// Set the sort column ID to nameCol
	model->set_sort_column(nameColumn, Gtk::SORT_ASCENDING);

	// Then apply the custom sort function, bind the folder column to the functor
	model->set_sort_func(nameColumn, sigc::bind(sigc::ptr_fun(sortFuncFoldersFirst), nameColumn, isFolderColumn));
}

} // namespace gtkutil
