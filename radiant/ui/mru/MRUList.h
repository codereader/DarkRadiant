#ifndef MRULIST_H_
#define MRULIST_H_

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/sequenced_index.hpp>

/* greebo: The MRUList maintains the list of filenames in a FIFO-style 
 * boost::multi_index_container of length _numMaxItems. 
 * 
 * Construct it with the maximum number of strings this list can hold.
 * 
 * Use insert() to add a filename to the list. Duplicated filenames are
 * recognised and relocated to the top of the list.
 * 
 * This is adapted from:
 * http://www.boost.org/libs/multi_index/example/serialization.cpp
 */
namespace ui {

class MRUList {
	
	/* greebo: This is the (rather complex) type definition of the
	 * list containing the filenames of type std::string 
	 */
	typedef boost::multi_index::multi_index_container<
				std::string,
				boost::multi_index::indexed_by<
					boost::multi_index::sequenced<>,
					boost::multi_index::hashed_unique<boost::multi_index::identity<std::string> >
					>
				> FileList;

	std::size_t _numMaxItems;

	// The actual list
	FileList _list;

public:
	// The public iterator, to make this class easier to use
	typedef FileList::iterator iterator;

	// Constructor
	MRUList(std::size_t numMaxItems) :
		_numMaxItems(numMaxItems) 
	{}

	void insert(const std::string& filename) {
		std::pair<iterator, bool> p = _list.push_front(filename);
	
		if (!p.second) {                     /* duplicate item */
			_list.relocate(_list.begin(), p.first); /* put in front */
		}
		else if (_list.size()>_numMaxItems) {  /* keep the length <= _numMaxItems */
			_list.pop_back();
		}
	}

	iterator begin() {
		return _list.begin();
	}
	
	iterator end() {
		return _list.end();
	}
	
	bool empty() const {
		return (_list.begin() == _list.end());
	}

}; // class MRUList

} // namespace ui

#endif /*MRULIST_H_*/
