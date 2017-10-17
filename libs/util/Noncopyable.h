#pragma once

namespace util
{

/**
 * Classes deriving from this one will end up as noncopyable
 * since the copy-constructor and assignment operators are marked
 * as deleted. Modeled after boost::noncopyable.
 */
class Noncopyable
{
public:
	constexpr Noncopyable() = default;

protected:
	Noncopyable(const Noncopyable&) = delete;
	Noncopyable& operator=(const Noncopyable&) = delete;
};

}
