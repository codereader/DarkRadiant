#ifndef SORTEDFILENAMES_H_
#define SORTEDFILENAMES_H_

/**
 * greebo: SortedFilenames is based on a std::set 
 *         container with special sorting.
 * 
 * The list fills itself with filenames while traversing a directory.
 * The class acts as File functor for the Directory_Foreach method.
 */

// Arnout: note - sort pakfiles in reverse order. This ensures that
// later pakfiles override earlier ones. This because the vfs module
// returns a filehandle to the first file it can find (while it should
// return the filehandle to the file in the most overriding pakfile, the
// last one in the list that is).
class PakLess
{
public:
	inline int ascii_to_upper(int c) const {
		if (c >= 'a' && c <= 'z') {
			return c - ('a' - 'A');
		}
		return c;
	}
	
	/*!
		This behaves identically to stricmp(a,b), except that ASCII chars
		[\]^`_ come AFTER alphabet chars instead of before. This is because
		it converts all alphabet chars to uppercase before comparison,
		while stricmp converts them to lowercase.
	*/
	bool operator()(const std::string& self, const std::string& other) const {
		const char* a = self.c_str();
		const char* b = other.c_str();
		
		for (;;) {
			int c1 = ascii_to_upper(*a++);
			int c2 = ascii_to_upper(*b++);

			if (c1 < c2) {
				return false; // a < b
			}

			if (c1 > c2) {
				return true; // a > b
			}

			if (c1 == 0) {
				// greebo: End of first string reached, strings are equal
				return false; // a == b && a == 0
			}
		}
	}
};

/**
 * greebo: A container providing a File functor method to populate
 *         itself with the visited filenames.
 * 
 *         The inserted filenames get correctly sorted on insert, as
 *         this class is using the PakLess comparator.
 */
class SortedFilenames :
	public std::set<std::string, PakLess>
{
public:
	// This gets called by Directory_Foreach visiting each filename.
	void operator()(const std::string& filename) {
		// Just insert the name into <self>, it will get sorted correctly.
		insert(filename);
	}
};

#endif /*SORTEDFILENAMES_H_*/
