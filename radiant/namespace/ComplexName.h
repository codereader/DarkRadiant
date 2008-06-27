#ifndef _COMPLEX_NAME_H_
#define _COMPLEX_NAME_H_

#include <string>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "string/string.h"

class ComplexName
{
	std::string _name;
	std::string _postfix;

	// The actual number. -1 is considered as invalid/empty postfix
	int _postfixNumber;

public:
	ComplexName(const std::string& fullname) {
		// Retrieve the name by cutting off the trailing number
		_name = boost::algorithm::trim_right_copy_if(
			fullname, boost::algorithm::is_any_of("1234567890")
		);

		// Get the trimmed part and take it as postfix 
		_postfix = fullname.substr(_name.size());

		// Convert this to a number
		_postfixNumber = strToInt(_postfix, -1);
	}

	std::string getFullname() const {
		return _name + _postfix;
	}
	
	const std::string& getNameWithoutPostfix() const {
		return _name;
	}

	const std::string& getPostfix() const {
		return _postfix;
	}

	int getPostfixNumber() const {
		return _postfixNumber;
	}

	void setPostfix(int postfix) {
		_postfixNumber = postfix;
		_postfix = (postfix == -1) ? "" : intToStr(postfix);
	}

	/**
	 * greebo: Changes the postfix of this name to make it unique,
	 * based on the information found in the given PostfixSet.
	 *
	 * After this call, the new postfix has been inserted into the set.
	 *
	 * @returns: the new postfix number
	 **/
	int makeUnique(PostfixSet& postfixes) {
		// Check if "our" number is already in the given set
		if (postfixes.find(_postfixNumber) != postfixes.end()) {
			// Yes, find a new free number and assign it
			setPostfix(postfixes.getUniqueAndInsert());
		}
		else {
			// The number is not yet in that set, insert it
			postfixes.insert(_postfixNumber);
		}

		return _postfixNumber;
	}
};

#endif /* _COMPLEX_NAME_H_ */
