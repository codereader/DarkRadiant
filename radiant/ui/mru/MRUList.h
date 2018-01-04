#pragma once

#include <list>

namespace ui
{

/* greebo: The MRUList maintains the recent file paths in a FIFO-style
 * container of length _numMaxItems.
 *
 * Construct it with the maximum number of strings this list can hold.
 *
 * Use insert() to add a filename to the list. Duplicated filenames are
 * recognised and relocated to the top of the list.
 */
class MRUList
{
private:
	typedef std::list<std::string> FileList;

	std::size_t _numMaxItems;

	// The actual list
	FileList _list;

public:
	// The public iterator, to make this class easier to use
	typedef FileList::iterator iterator;
	typedef FileList::const_iterator const_iterator;

	// Constructor
	MRUList(std::size_t numMaxItems) :
		_numMaxItems(numMaxItems)
	{}

	void insert(const std::string& filename)
	{
		// Check if the filename is already in the list
		iterator existing = std::find(_list.begin(), _list.end(), filename);

		if (existing != _list.end())
		{
			// Relocate to top of the list and be done
			_list.splice(_list.begin(), _list, existing);
			return;
		}

		// Not present yet, insert at front of the list
		_list.push_front(filename);
		
		// keep the length <= _numMaxItems
		if (_list.size() > _numMaxItems) 
		{  
			_list.pop_back();
		}
	}

	const_iterator begin() const
	{
		return _list.begin();
	}

	const_iterator end() const
	{
		return _list.end();
	}

	iterator begin()
	{
		return _list.begin();
	}

	iterator end()
	{
		return _list.end();
	}

	bool empty() const
	{
		return _list.empty();
	}

}; // class MRUList

} // namespace
