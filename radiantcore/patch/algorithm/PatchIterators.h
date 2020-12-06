#pragma once

#include "ipatch.h"
#include <fmt/format.h>

namespace patch
{

/**
 * Forward iterator used to traverse the control points of a patch.
 * The visited points, stride or ordering are defined by the forward function
 * object passed to the constructor.
 *
 * Each iterator instance refers to a single control of the patch, defined
 * by the row and col indices. An iterator whose coordinates are out of bounds
 * will return isValid() == false, throwing exceptions when dereferenced.
 *
 * The state of the iterator is protected allowing subclasses to access
 * it in their forward function implementation.
 */
class PatchControlIterator
{
public:
    // The function object implementing the move-forward logic of this iterator
    using Forwarder = std::function<void(PatchControlIterator&)>;

protected:
    IPatch& _patch;

    // Using signed indices to allow out-of-bounds iterators
    int _row;
    int _col;

    // Moves this iterator forward by one step
    Forwarder _forward;

protected:
    PatchControlIterator(IPatch& patch, int row, int col, Forwarder forward) :
        _patch(patch),
        _row(row),
        _col(col),
        _forward(forward)
    {}

    PatchControlIterator(const PatchControlIterator& other) = default;
    PatchControlIterator& operator=(const PatchControlIterator& other) = default;

public:
    // Post-increment i++
    PatchControlIterator operator++(int)
    {
        PatchControlIterator previous = *this;
        ++(*this);
        return previous;
    }

    // Pre-increment ++i
    PatchControlIterator& operator++()
    {
        // Call the logic moving our iterator forward by one step
        _forward(*this);

        return *this;
    }

    PatchControl& operator*()
    {
        throwIfOutOfBounds();
        return _patch.ctrlAt(static_cast<std::size_t>(_row), static_cast<std::size_t>(_col));
    }

    const PatchControl& operator*() const
    {
        throwIfOutOfBounds();
        return _patch.ctrlAt(_row, _col);
    }

    PatchControl* operator->()
    {
        return &**this;
    }

    const PatchControl* operator->() const
    {
        return &**this;
    }

    bool isValid() const
    {
        return _col >= 0 && _row >= 0 &&
                _col < static_cast<int>(_patch.getWidth()) &&
                _row < static_cast<int>(_patch.getHeight());
    }

    // Interface to set or reset the iterator coordinates
    int getRow() const
    {
        return _row;
    }

    int getColumn() const
    {
        return _col;
    }

    void set(int row, int col)
    {
        _row = row;
        _col = col;
    }

private:
    void throwIfOutOfBounds() const
    {
        // Dereferencing an out-of-bounds iterator?
        if (!isValid())
        {
            throw std::runtime_error(fmt::format("Iterator (row={0},col={1}) is out of bounds", _row, _col));
        }
    }
};

// An iterator traversing a single row of a patch
class PatchRowIterator :
    public PatchControlIterator
{
public:
    PatchRowIterator(IPatch& patch, std::size_t row) :
        PatchControlIterator(patch, static_cast<int>(row), 0, moveToNextCol)
    {}

private:
    static void moveToNextCol(PatchControlIterator& it)
    {
        it.set(it.getRow(), it.getColumn() + 1);
    }
};

// An iterator traversing a single column of a patch
class PatchColumnIterator :
    public PatchControlIterator
{
public:
    PatchColumnIterator(IPatch& patch, std::size_t col) :
        PatchControlIterator(patch, 0, static_cast<int>(col), moveToNextRow)
    {}

private:
    static void moveToNextRow(PatchControlIterator& it)
    {
        it.set(it.getRow() + 1, it.getColumn());
    }
};

}
