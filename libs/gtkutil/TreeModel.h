#ifndef TREEMODEL_H_
#define TREEMODEL_H_

#include <string>
#include <gtkmm/treesortable.h>
#include <gtkmm/treemodel.h>

namespace Gtk { class TreeView; }

namespace gtkutil
{

/**
 * Utility class for operation on Gtk::TreeModels.
 */
class TreeModel
{
public:
	/**
	 * greebo: Utility callback for use in Gtk::TreeView::set_search_equal_func, which
	 * enables some sort of "full string" search in treeviews.
	 *
	 * The equalFuncStringContains returns "match" as soon as the given key occurs
	 * somewhere in the string in question, not only at the beginning (GTK default).
	 *
	 * Prerequisites: The search column must contain a string.
	 */
	static bool equalFuncStringContains(const Glib::RefPtr<Gtk::TreeModel>& model,
										int column,
										const Glib::ustring& key,
										const Gtk::TreeModel::iterator& iter);

	class SelectionFinder
	{
	private:
		// String containing the name to highlight
		std::string _selection;

		// An integer to search for (alternative to the string above)
		int _needle;

		// The found iterator
		Gtk::TreeModel::iterator _foundIter;

		// The column index to be searched
		int _column;

		// TRUE, if this should search for an integer instead of a string
		bool _searchForInt;

	public:

		// Constructor to search for strings
		SelectionFinder(const std::string& selection, int column);

		// Constructor to search for integers
		SelectionFinder(int needle, int column);

		/** greebo: Get the GtkTreeIter corresponding to the found path.
		 * 			The returnvalue can be invalid if the internal found path is NULL.
		 */
		const Gtk::TreeModel::iterator getIter() const;

		// Callback for gtkmm
		bool forEach(const Gtk::TreeModel::iterator& iter);
	};

	/**
	 * greebo: Tries to lookup the given string in the given column of the given view.
	 * Returns TRUE if the lookup and the selection was successful, FALSE otherwise.
	 */
	static bool findAndSelectString(Gtk::TreeView* view,
									const std::string& needle,
									int column);

	/**
	 * greebo: Tries to lookup the given string in the given column of the given view.
	 * Returns TRUE if the lookup and the selection was successful, FALSE otherwise.
	 */
	static bool findAndSelectInteger(Gtk::TreeView* view, int needle, int column);

	/**
	 * greebo: Tries to lookup the given string in the given column of the given view.
	 * Returns TRUE if the lookup and the selection was successful, FALSE otherwise.
	 */
	static bool findAndSelectString(Gtk::TreeView* view,
									const std::string& needle,
									const Gtk::TreeModelColumn<Glib::ustring>& column);

	/**
	 * greebo: Tries to lookup the given string in the given column of the given view.
	 * Returns TRUE if the lookup and the selection was successful, FALSE otherwise.
	 */
	static bool findAndSelectString(Gtk::TreeView* view,
									const std::string& needle,
									const Gtk::TreeModelColumn<std::string>& column);

	/**
	 * greebo: Tries to lookup the given string in the given column of the given view.
	 * Returns TRUE if the lookup and the selection was successful, FALSE otherwise.
	 */
	static bool findAndSelectInteger(Gtk::TreeView* view, int needle,
									 const Gtk::TreeModelColumn<int>& column);

	/**
	 * greebo: Takes care that the given tree model is sorted such that
	 * folders are listed before "regular" items.
	 *
	 * @model: The tree model to sort, must implement GtkTreeSortable.
	 * @nameCol: the column number containing the name
	 * @isFolderColumn: the column number containing a boolean flag: "is folder"
	 */
	static void applyFoldersFirstSortFunc(const Glib::RefPtr<Gtk::TreeSortable>& model,
										  const Gtk::TreeModelColumn<Glib::ustring>& nameColumn,
										  const Gtk::TreeModelColumn<bool>& isFolderColumn);

	static void applyFoldersFirstSortFunc(const Glib::RefPtr<Gtk::TreeSortable>& model,
										  const Gtk::TreeModelColumn<std::string>& nameColumn,
										  const Gtk::TreeModelColumn<bool>& isFolderColumn);

private:
	static int sortFuncFoldersFirstStd(const Gtk::TreeModel::iterator& a,
									const Gtk::TreeModel::iterator& b,
									const Gtk::TreeModelColumn<std::string>& nameColumn, // columns are bound
									const Gtk::TreeModelColumn<bool>& isFolderColumn);	 // by applyFoldersFirstSortFunc

	static int sortFuncFoldersFirst(const Gtk::TreeModel::iterator& a,
									const Gtk::TreeModel::iterator& b,
									const Gtk::TreeModelColumn<Glib::ustring>& nameColumn, // columns are bound
									const Gtk::TreeModelColumn<bool>& isFolderColumn);	 // by applyFoldersFirstSortFunc
};

} // namespace gtkutil

#endif /*TREEMODEL_H_*/
