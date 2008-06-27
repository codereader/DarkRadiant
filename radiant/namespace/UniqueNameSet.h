#ifndef _UNIQUE_NAME_SET_H_
#define _UNIQUE_NAME_SET_H_

#include <set>
#include <map>

#include "PostfixSet.h"
#include "ComplexName.h"

class UniqueNameSet
{
	// This maps name prefixes to a set of used postfixes
	// e.g. "func_static_" => [1,3,4,5,10]
	// Allows fairly quick lookup of used names and postfixes
	typedef std::map<std::string, PostfixSet> Names;
	Names _names;

public:
	bool empty() const {
		// Cycle through all prefixes and see if the postfixset is non-empty, break on first hit
		for (Names::const_iterator i = _names.begin(); i != _names.end(); i++) {
			if (!i->second.empty()) {
				return false;
			}
		}

		return true;
	}

	/** 
	 * Inserts the given full name into this set. Returns TRUE if the insertion
	 * was successful (i.e. the name didn't exist yet).
	 */
	bool insert(const std::string& fullName) {
		return insert(ComplexName(fullName));
	}

	/**
	 * Removes the given full name from the set. Returns TRUE if the name
	 * was there in the first place.
	 */
	bool erase(const std::string& fullName) {
		return erase(ComplexName(fullName));
	}

	// Same as insert, but with a ComplexName as input
	bool insert(const ComplexName& name) {
		// Lookup the name in the map to see if we know this prefix already
		Names::iterator found = _names.find(name.getNameWithoutPostfix());

		if (found == _names.end()) {
			// The name is not yet in the list, insert it afresh 
			std::pair<Names::iterator, bool> result = _names.insert(
				Names::value_type(name.getNameWithoutPostfix(), PostfixSet())
			);

			assert(result.second); // insert must succeed, we didn't find this just before

			// Overwrite the iterator
			found = result.first;
		}

		// The prefix is inserted at this point, add the postfix to the set
		PostfixSet& postfixSet = found->second;

		std::pair<PostfixSet::iterator, bool> result = postfixSet.insert(
			name.getPostfixNumber()
		);

		// Return the boolean of the insertion result, it is true on successful insertion
		return result.second;
	}

	// Same as erase, but with a ComplexName as input
	bool erase(const ComplexName& name) {
		// Lookup the name in the map to see if we know this prefix already
		Names::iterator found = _names.find(name.getNameWithoutPostfix());

		if (found == _names.end()) {
			return false; // Not found!
		}
	
		// The prefix has been found, remove the postfix from the set
		PostfixSet& postfixSet = found->second;

		// Return true if the std::set::erase method removed any elements
		return (postfixSet.erase(name.getPostfixNumber()) > 0);
	}

	/**
	 * greebo: Changes the given ComplexName to a unique name,
	 * which is usually achieved by setting the postfix number to
	 * an unused one. The new, unique name is automatically inserted
	 * into the map of known names.
	 */
	void makeUniqueAndInsert(ComplexName& name) {
		// Lookup the name in the map to see if we know this prefix
		Names::iterator found = _names.find(name.getNameWithoutPostfix());

		if (found == _names.end()) {
			// The name is not yet in the list, we can add it with the given postfix
			std::pair<Names::iterator, bool> result = _names.insert(
				Names::value_type(name.getNameWithoutPostfix(), PostfixSet())
			);

			assert(result.second); // insert must succeed, we didn't find this just before

			// Overwrite the iterator
			found = result.first;
		}

		// At this point, the "trunk" of the complex name is already in the list
		// The found iterator points to a valid prefix => PostFixSet mapping

		// Acquire a new unique postfix for this name to make it unique
		PostfixSet& postfixSet = found->second;
		name.makeUnique(postfixSet);
	}

	/**
	 * greebo: Changes the given ComplexName to a unique name,
	 * which is usually achieved by setting the postfix number to
	 * an unused one. Doesn't change the existing set.
	 */
	void makeUnique(ComplexName& name) {
		// Lookup the name in the map to see if we know this prefix
		Names::iterator found = _names.find(name.getNameWithoutPostfix());

		if (found == _names.end()) {
			// The name is not yet in the list, return the name as it is
			return;
		}

		// The found iterator points to a valid prefix => PostfixSet mapping
		// Find a unique postfix for this name without altering the PostfixSet
		name.setPostfix(found->second.getUnique());
	}

	/**
	 * greebo: Returns true if the full name already exists in this set.
	 */
	bool nameExists(const std::string& fullname) const {
		// Empty names never exist
		if (fullname.empty()) {
			return false;
		}

		// Analyse the name (split it into parts) and pass the call to the specialised method
		return nameExists(ComplexName(fullname));
	}

	/**
	 * greebo: Returns true if the complex name already exists in this set.
	 */
	bool nameExists(const ComplexName& name) const {
		// Lookup the name in the map to see if we know this prefix
		Names::const_iterator found = _names.find(name.getNameWithoutPostfix());

		if (found != _names.end()) {
			// We know the name "trunk", does the number exist?
			const PostfixSet& postfixSet = found->second;
			
			// If we know the number too, the full name exists
			return (postfixSet.find(name.getPostfixNumber()) != postfixSet.end());
		}
		else {
			// Prefix is not known, hence full name is not known
			return false;
		}
	}

	/** 
	 * Copies all names from the <other> UniqueNameSet into this one.
	 * This class will contain the union of both sets afterwards. 
	 * Duplicate names will be ignored.
	 */
	void merge(const UniqueNameSet& other) {
		// cycle through all foreign names and import them
		for (Names::const_iterator i = other._names.begin(); i != other._names.end(); i++) {
			// Check if the prefix exists already in this set
			Names::iterator local = _names.find(i->first);

			if (local != _names.end()) {
				// Prefix exists, merge the postfixes
				local->second.insert(i->second.begin(), i->second.end());
			}
			else {
				// Prefix doesn't exist yet, insert the whole string => PostfixSet pair
				_names.insert(*i);
			}
		}
	}
};

#endif /* _UNIQUE_NAME_SET_H_ */
