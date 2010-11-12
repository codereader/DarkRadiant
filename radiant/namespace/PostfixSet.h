#ifndef _POSTFIX_SET_H_
#define _POSTFIX_SET_H_

#include <set>
#include <limits.h>

#define LOWEST_POSTFIX_NUMBER 1

/**
 * greebo: A PostfixSet extends the std::set interface
 * by two utility functions to retrieve unused postfixes.
 */
class PostfixSet :
	public std::set<int>
{
public:
	/**
	 * greebo: Returns a new number which doesn't belong to this set so far.
	 */
	int getUnique() const {
		// Yes, find a new free number, starting from 1
		for (int i = LOWEST_POSTFIX_NUMBER; i < INT_MAX; i++) {
			if (find(i) == end()) {
				// Found a free value
				return i;
			}
		}

		// This is the pathological case, but compilers want this
		return INT_MAX;
	}

	/**
	 * greebo: Same as getUnique(), but inserts the newly found value into
	 * the set before returning it.
	 */
	int getUniqueAndInsert() {
		// Yes, find a new free number, starting from 1
		for (int i = LOWEST_POSTFIX_NUMBER; i < INT_MAX; i++) {
			if (find(i) == end()) {
				// Found a free value, insert and return
				insert(i);
				return i;
			}
		}

		// This is the pathological case, but compilers want this
		return INT_MAX;
	}
};

#endif /* _POSTFIX_SET_H_ */
