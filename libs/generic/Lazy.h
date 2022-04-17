#pragma once

#include <optional>
#include <functional>

/// Wrapper for an arbitrary object which is initialised lazily via a functor
template<typename T> class Lazy
{
    // Contained value (may or may not be initialised)
    mutable std::optional<T> _value;

    // Functor to initialise _value when required
    using InitialiseFunc = std::function<T()>;
    InitialiseFunc _initFunc;

public:

    /// Construct the Lazy wrapper with a functor to be called when initialisation is required
    Lazy(InitialiseFunc func): _initFunc(func)
    {}

    /// Return the contained value, initialising via the functor if necessary
    T get() const
    {
        if (!_value)
            _value = _initFunc();

        return *_value;
    }
};