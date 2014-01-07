#pragma once

#include "idatastream.h"
#include "iarchive.h"
#include <memory>

/**
 * Scoped class reading all the data from the attached
 * ArchiveFile into a single memory chunk. Clients usually 
 * refer to the buffer variable to access the data.
 */
class ScopedArchiveBuffer
{
private:
	// unique_ptr has a specialisation for arrays
	std::unique_ptr<InputStream::byte_type[]> data;

public:
	InputStream::byte_type* const buffer; // immutable pointer for convenience purposes
	std::size_t length;
	
	ScopedArchiveBuffer(ArchiveFile& file) :
		data(new InputStream::byte_type[file.size() + 1]),
		buffer(data.get())
	{
		length = file.getInputStream().read(data.get(), file.size());
		data[file.size()] = 0;
	}
};
