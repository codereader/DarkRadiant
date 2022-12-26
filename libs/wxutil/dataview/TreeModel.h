#pragma once

#include <wx/dataview.h>
#include <functional>
#include <vector>
#include <memory>
#include "util/Noncopyable.h"

namespace wxutil
{

/**
 * Implements a wxwidgets DataViewModel, similar to wxDataViewTreeStore
 * but with more versatility of the stored columns.
 */
class TreeModel :
	public wxDataViewModel
{
public:

    /// Reference-counted smart pointer type
    typedef wxObjectDataPtr<TreeModel> Ptr;

    class ColumnRecord;

	/**
	 * Represents a column in the wxutil::TreeModel
	 * Use the Type string to instantiate a Column and arrange
	 * multiple columns into a ColumnRecord structure.
	 */
	class Column
	{
	public:
		enum Type
		{
			String = 0,
			Integer,
			Double,
			Boolean,
			Icon,
			IconText,
			Pointer,
			NumTypes
		};

		Type type;
		std::string name;

	private:
		// column index in the treemodel - this member is assigned
		// by the TreeModel itself on attachment
		int _col;

	public:
		Column(Type type_, const std::string& name_ = "") :
			type(type_),
			name(name_),
			_col(-1)
		{}

		// Returns the index of this column
		int getColumnIndex() const
		{
			if (_col == -1) throw std::runtime_error("Cannot query column index of unattached column.");

			return _col;
		}

		// Returns the wxWidgets type string of this column
		wxString getWxType() const;

    private:
		// Only for internal use by the ColumnRecord friend
		void setColumnIndex(int index)
		{
			_col = index;
		}

        friend class ColumnRecord;
	};

	/**
	 * Use this record to declare the column order of the TreeModel.
	 * Subclasses should call Add() for each of their Column members.
	 */
	class ColumnRecord :
		public util::Noncopyable
	{
	public:
		// A list of column references, these point to members of subclasses
		typedef std::vector<Column> List;

	private:
		List _columns;

	protected:
		// Only to be instantiated by subclasses
		ColumnRecord() {}

	public:
		Column add(Column::Type type, const std::string& name = "")
		{
            _columns.emplace_back(type, name);
            auto& column = _columns.back();
            column.setColumnIndex(static_cast<int>(_columns.size()) - 1);

			return column;
		}

		List::iterator begin()
		{ 
			return _columns.begin();
		}

		List::const_iterator begin() const
		{ 
			return _columns.begin();
		}

		List::iterator end()
		{ 
			return _columns.end();
		}

		List::const_iterator end() const
		{ 
			return _columns.end();
		}

		List::size_type size() const
		{ 
			return _columns.size();
		}

		const List::value_type& operator[](std::size_t index) const
		{
			return _columns[index];
		}

		List::value_type& operator[](std::size_t index)
		{
			return _columns[index];
		}
	};

	// An assignment helper, for use in TreeModel::Row
	class ItemValueProxy
	{
	private:
		wxDataViewItem _item;
		const Column& _column;
		TreeModel& _model;

	public:
		ItemValueProxy(const wxDataViewItem& item, const Column& column, TreeModel& model) :
			_item(item),
			_column(column),
			_model(model)
		{}

		ItemValueProxy(const ItemValueProxy& other) :
			_item(other._item),
			_column(other._column),
			_model(other._model)
		{}

		// get/set operators
		ItemValueProxy& operator=(const wxVariant& data)
		{
            _model.SetValue(data, _item, _column.getColumnIndex());

			// Newly assigned values are enabled by default
			_model.SetEnabled(_item, _column.getColumnIndex(), true);

			return *this;
		}

		// get/set operators for dataview attributes
		ItemValueProxy& setAttr(const wxDataViewItemAttr& attr)
		{
			_model.SetAttr(_item, _column.getColumnIndex(), attr);
			return *this;
		}

		wxVariant getVariant() const
		{
			wxVariant variant;

			_model.GetValue(variant, _item, _column.getColumnIndex());

			return variant;
		}

		operator wxVariant() const
		{
			return getVariant();
		}

		// IconText columns are widely used, let's provide an operator cast
		operator wxDataViewIconText() const
		{
			wxDataViewIconText iconText;
			iconText << getVariant();

			return iconText;
		}

		// Don't implement operator bool() directly, it can be converted to integer
		bool getBool() const
		{
            bool value;
            return getVariant().Convert(&value) ? value : false;
		}

		int getInteger() const
		{
            long value;
            return getVariant().Convert(&value) ? static_cast<int>(value) : 0;
		}

		double getDouble() const
		{
            double value;
            return getVariant().Convert(&value) ? value : 0.0;
		}

		void* getPointer() const
		{
			return getVariant().IsNull() ? nullptr : getVariant().GetVoidPtr();
		}

        // Extract the string value (works for Column::String and Column::IconText)
        wxString getString() const
        {
            if (_column.type == Column::String)
            {
                auto variant = getVariant();
                return variant.IsNull() ? wxString() : variant.GetString();
            }
            else if (_column.type == Column::IconText)
            {
                wxDataViewIconText iconText;
                iconText << getVariant();

                return iconText.GetText();
            }

            return wxString();
        }

		operator std::string() const
		{
            return getString().ToStdString();
		}

		bool isEnabled()
		{
			return _model.IsEnabled(_item, _column.getColumnIndex());
		}

		// Enable/disable the column value, by default values are enabled after assignment
		// This is adhered by some cell renderers which draw the value greyed out or inactive
		void setEnabled(bool enabled)
		{
			_model.SetEnabled(_item, _column.getColumnIndex(), enabled);
		}
	};

	/**
	 * A convenience representation of a single wxDataViewItem,
	 * allowing to set column values using the operator[].
	 */
	class Row
	{
	private:
		wxDataViewItem _item;
		TreeModel& _model;

	public:
		Row(const wxDataViewItem& item, wxDataViewModel& model) :
			 _item(item),
			 _model(static_cast<TreeModel&>(model))
		{
			assert(dynamic_cast<TreeModel*>(&_model) != NULL);
		}

		const wxDataViewItem& getItem() const
		{
			return _item;
		}

		const ItemValueProxy operator[](const Column& column) const
		{
			return ItemValueProxy(_item, column, _model);
		}

		ItemValueProxy operator[](const Column& column)
		{
			return ItemValueProxy(_item, column, _model);
		}

		// Sends the ItemAdded event to the associated treestore
		void SendItemAdded()
		{
			_model.ItemAdded(_model.GetParent(_item), _item);
		}

		void SendItemChanged()
		{
			_model.ItemChanged(_item);
		}

		void SendItemDeleted()
		{
			_model.ItemDeleted(_model.GetParent(_item), _item);
		}
	};

	// Visit function
	typedef std::function<void(Row&)> VisitFunction;

	// Sort function - should return true if a < b, false otherwise
	typedef std::function<bool (const wxDataViewItem&, const wxDataViewItem&)> SortFunction;

	// Event to be emitted by threaded treemodel populators. Worker threads should use events
	// to communicate with the main GUI thread.
	class PopulationFinishedEvent : 
		public wxEvent
	{
	private:
        TreeModel::Ptr _treeModel;

	public:
		PopulationFinishedEvent(int id = 0);
		PopulationFinishedEvent(TreeModel::Ptr store, int id = 0);
		PopulationFinishedEvent(const PopulationFinishedEvent& event);
 
		wxEvent* Clone() const;
 
        TreeModel::Ptr GetTreeModel() const;
		void SetTreeModel(TreeModel::Ptr store);
	};

	typedef void (wxEvtHandler::*PopulationFinishedFunction)(PopulationFinishedEvent&);

    // During population some threads send progress information to their connected event handlers
    class PopulationProgressEvent :
        public wxEvent
    {
    private:
        wxString _message;

    public:
        PopulationProgressEvent(int id = 0);
        PopulationProgressEvent(const wxString& message, int id = 0);
        PopulationProgressEvent(const PopulationProgressEvent& event);

        wxEvent* Clone() const;

        const wxString& GetMessage() const;
        void SetMessage(const wxString& message);
    };

    typedef void (wxEvtHandler::*PopulationProgressFunction)(PopulationProgressEvent&);

protected:
	class Node;
	typedef std::shared_ptr<Node> NodePtr;

	class SearchFunctor;

private:
	const ColumnRecord& _columns;

	NodePtr _rootNode;

	int _defaultStringSortColumn;

	bool _hasDefaultCompare;
	bool _isListModel;

protected:
	// Constructor to be used by subclasses, allows an existing model to be referenced.
	// The root node of the existing model will be shared by this instance.
	// This is not a copy constructor btw.
	TreeModel(const TreeModel& existingModel);

public:
	TreeModel(const ColumnRecord& columns, bool isListModel = false);

	~TreeModel() override;

	// Return the column definition of this model
	const ColumnRecord& GetColumns() const;

	// Add a new item below the root element
	Row AddItem();

	/// Add a new item below the given element
	Row AddItemUnderParent(const wxDataViewItem& parent);

	// Removes the item, returns TRUE on success
	bool RemoveItem(const wxDataViewItem& item);

	// Remove all items matching the predicate, returns the number of deleted items
	int RemoveItems(const std::function<bool (const Row&)>& predicate);

	// Returns a Row reference to the topmost element
	Row GetRootItem();

	// Returns true if the given column value should render as "enabled" or not
	bool IsEnabled(const wxDataViewItem& item, unsigned int col) const override;

	// Removes all items - internally the root node will be kept, but cleared too
	// This also fires the "Cleared" event to any listeners
	void Clear();

	void SetDefaultStringSortColumn(int index);
	void SetHasDefaultCompare(bool hasDefaultCompare);

	// Visit each node in the model, excluding the internal root node
	void ForeachNode(const VisitFunction& visitFunction);

	// Visit each node in the model, backwards direction, excluding the internal root node
	void ForeachNodeReverse(const TreeModel::VisitFunction& visitFunction);

	// Sorts the entire tree using the given sort function
	void SortModel(const SortFunction& sortFunction);

	// Sorts the entire tree by the given column (can also be a IconText column)
	void SortModelByColumn(const TreeModel::Column& column);

	// Sort the model by a string-valued column, sorting folders on top.
	// Pass a boolean-valued "is-a-folder" column to indicate which items are actual folders.
	void SortModelFoldersFirst(const Column& stringColumn, const Column& isFolderColumn);

    // Sort the model by a string-valued column, sorting folders on top, starting at the given item.
    // See also the other SortModelFoldersFirst overload.
    void SortModelFoldersFirst(const wxDataViewItem& startItem, const Column& stringColumn, const Column& isFolderColumn);

    using FolderCompareFunction = std::function<int(const wxDataViewItem&, const wxDataViewItem&)>;

    // Sort the model by a string-valued column, sorting folders on top.
    // Pass a boolean-valued "is-a-folder" column to indicate which items are actual folders.
    // The customFolderSortFunc can be used to compare folders in a user-defined way
    // if the custom folder func returns equal (0), the regular name comparison is performed.
	void SortModelFoldersFirst(const Column& stringColumn, const Column& isFolderColumn, 
        const FolderCompareFunction& customFolderSortFunc);

    // Sort the model by a string-valued column, sorting folders on top, starting at the given item.
    // See also the other SortModelFoldersFirst overload.
    void SortModelFoldersFirst(const wxDataViewItem& startItem, const Column& stringColumn, 
        const Column& isFolderColumn, const FolderCompareFunction& customFolderSortFunc);

    // Find the given string needle in the given column (searches the entire tree)
	wxDataViewItem FindString(const std::string& needle, const Column& column);

    // Find the given string needle in the given column (searches only the subtree given by the startNode item)
	wxDataViewItem FindString(const std::string& needle, const Column& column, const wxDataViewItem& startNode);

    // Find the given number needle in the given column (searches the entire tree)
	wxDataViewItem FindInteger(long needle, const Column& column);
    
    // Find the given number needle in the given column (searches only the subtree given by the startNode item)
	wxDataViewItem FindInteger(long needle, const Column& column, const wxDataViewItem& startNode);

    // Find the item matching the predicate (searches the entire tree)
    wxDataViewItem FindItem(const std::function<bool(const TreeModel::Row&)>& predicate);

    // Find the item matching the predicate (searches from the given item)
    wxDataViewItem FindItem(const std::function<bool(const TreeModel::Row&)>& predicate, const wxDataViewItem& startNode);

    // Returns true if any of the given columns in the given row contains the string value
    // Set lowerStrings to true to convert the column values to lowercase first (the value is not touched
    // and needs to be made lowercase by the calling code).
    static bool RowContainsString(const Row& row, const wxString& value, 
        const std::vector<Column>& columnsToSearch, bool lowerStrings = true);

	void SetAttr(const wxDataViewItem& item, unsigned int col, const wxDataViewItemAttr& attr) const;
	void SetIsListModel(bool isListModel);

	// Search for an item in the given columns (forward), using previousMatch as reference point
	// search is performed case-insensitively, partial matches are considered ("contains")
	wxDataViewItem FindNextString(const wxString& needle, 
		const std::vector<Column>& columns, const wxDataViewItem& previousMatch = wxDataViewItem());
	
	// Search for an item in the given columns (backwards), using previousMatch as reference point 
	// search is performed case-insensitively, partial matches are considered ("contains")
	wxDataViewItem FindPrevString(const wxString& needle,
		const std::vector<Column>& columns, const wxDataViewItem& previousMatch = wxDataViewItem());

	// Marks a specific column value as enabled or disabled.
	void SetEnabled(const wxDataViewItem& item, unsigned int col, bool enabled);

	// Base class implementation / overrides

	bool HasDefaultCompare() const override;
	unsigned int GetColumnCount() const override;

    // return type as reported by wxVariant
    wxString GetColumnType(unsigned int col) const override;

    // get value into a wxVariant
    void GetValue(wxVariant &variant,
                          const wxDataViewItem &item, unsigned int col) const override;
	bool SetValue(const wxVariant &variant,
                          const wxDataViewItem &item,
                          unsigned int col) override;

	bool GetAttr(const wxDataViewItem& item, unsigned int col, wxDataViewItemAttr& attr) const override;

	wxDataViewItem GetParent(const wxDataViewItem &item) const override;
    bool IsContainer(const wxDataViewItem& item) const override;

    bool HasContainerColumns(const wxDataViewItem&) const override;

	unsigned int GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const override;
	wxDataViewItem GetRoot();

	bool IsListModel() const override;

	int Compare(const wxDataViewItem& item1, const wxDataViewItem& item2,
                        unsigned int column, bool ascending) const override;

    // Forces client data views to refresh this subtree by sending add/remove events
    // for each of the children of the given item
    void SendSubtreeRefreshEvents(wxDataViewItem& parentItem);

protected:
	// Returns a reference to the actual rootnode, only allowed for use in subclasses
	const NodePtr& getRootNode() const;

protected:
	void ForeachNodeRecursive(const TreeModel::NodePtr& node, const VisitFunction& visitFunction);
	void ForeachNodeRecursiveReverse(const TreeModel::NodePtr& node, const TreeModel::VisitFunction& visitFunction);
	void SortModelRecursively(Node* node, const TreeModel::SortFunction& sortFunction);

	// Sort functor for the SortModelFoldersFirst() method, uses the stringCompare method to compare the actual text values
    // Pass CompareStringVariants or CompareIconTextVariants as stringCompare.
	bool CompareFoldersFirst(const wxDataViewItem& a, const wxDataViewItem& b, 
                             const Column& stringColumn,
                             const std::function<int(const wxVariant&, const wxVariant&)>& stringCompareFunc,
                             const Column& isFolderCol,
                             const std::function<int(const wxDataViewItem&, const wxDataViewItem&)>& folderCompareFunc);

    static int CompareStringVariants(const wxVariant& a, const wxVariant& b);
    static int CompareIconTextVariants(const wxVariant& a, const wxVariant& b);

	wxDataViewItem FindRecursive(const TreeModel::Node& node, const std::function<bool (const TreeModel::Node&)>& predicate);
	wxDataViewItem FindRecursiveUsingRows(const TreeModel::Node& node, const std::function<bool (TreeModel::Row&)>& predicate);
	int RemoveItemsRecursively(const wxDataViewItem& parent, const std::function<bool (const Row&)>& predicate);
};

// wx event macros
wxDECLARE_EVENT(EV_TREEMODEL_POPULATION_FINISHED, TreeModel::PopulationFinishedEvent);
#define TreeModelPopulationFinishedHandler(func) wxEVENT_HANDLER_CAST(wxutil::TreeModel::PopulationFinishedFunction, func)

wxDECLARE_EVENT(EV_TREEMODEL_POPULATION_PROGRESS, TreeModel::PopulationProgressEvent);
#define TreeModelPopulationProgressHandler(func) wxEVENT_HANDLER_CAST(wxutil::TreeModel::PopulationProgressFunction, func)

} // namespace
