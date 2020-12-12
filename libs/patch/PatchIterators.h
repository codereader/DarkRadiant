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

public:
    PatchControlIterator(IPatch& patch, int row, int col, Forwarder forward) :
        _patch(patch),
        _row(row),
        _col(col),
        _forward(forward)
    {}

    PatchControlIterator(const PatchControlIterator& other) = default;
    PatchControlIterator& operator=(const PatchControlIterator& other) = default;

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

// An iterator traversing a single row of a patch (in defined order)
class SinglePatchRowIteratorBase :
    public PatchControlIterator
{
public:
    SinglePatchRowIteratorBase(IPatch& patch, std::size_t row, std::size_t startCol, int delta) :
        PatchControlIterator(patch, static_cast<int>(row), static_cast<int>(startCol), 
            std::bind(SinglePatchRowIteratorBase::moveToNextCol, std::placeholders::_1, delta))
    {}

private:
    static void moveToNextCol(PatchControlIterator& it, int delta)
    {
        it.set(it.getRow(), it.getColumn() + delta);
    }
};

// An iterator traversing a single row of a patch
class SinglePatchRowIterator :
    public SinglePatchRowIteratorBase
{
public:
    SinglePatchRowIterator(IPatch& patch, std::size_t row) :
        SinglePatchRowIteratorBase(patch, static_cast<int>(row), 0, +1)
    {}
};

// An iterator traversing a single row of a patch in reverse order (starting from the highest column)
class SinglePatchRowReverseIterator :
    public SinglePatchRowIteratorBase
{
public:
    SinglePatchRowReverseIterator(IPatch& patch, std::size_t row) :
        SinglePatchRowIteratorBase(patch, static_cast<int>(row), static_cast<int>(patch.getWidth() - 1), -1)
    {}
};

// An iterator traversing a single column of a patch (in defined order)
class SinglePatchColumnIteratorBase :
    public PatchControlIterator
{
public:
    SinglePatchColumnIteratorBase(IPatch& patch, std::size_t col, std::size_t startRow, int delta) :
        PatchControlIterator(patch, static_cast<int>(startRow), static_cast<int>(col), 
            std::bind(SinglePatchColumnIteratorBase::moveToNextRow, std::placeholders::_1, delta))
    {}

private:
    static void moveToNextRow(PatchControlIterator& it, int delta)
    {
        it.set(it.getRow() + delta, it.getColumn());
    }
};

// An iterator traversing a single column of a patch
class SinglePatchColumnIterator :
    public SinglePatchColumnIteratorBase
{
public:
    SinglePatchColumnIterator(IPatch& patch, std::size_t row) :
        SinglePatchColumnIteratorBase(patch, static_cast<int>(row), 0, +1)
    {}
};

// An iterator traversing a single column of a patch in reverse order (starting from the highest row)
class SinglePatchColumnReverseIterator :
    public SinglePatchColumnIteratorBase
{
public:
    SinglePatchColumnReverseIterator(IPatch& patch, std::size_t row) :
        SinglePatchColumnIteratorBase(patch, static_cast<int>(row), static_cast<int>(patch.getHeight() - 1), -1)
    {}
};

// An iterator traversing a given patch column-wise, iterating over
// one column after the other (which are optionally constrained to [startColumn..endColumn])
// in the given row direction (+1 == Forward, -1 == Backwards)
class ColumnWisePatchIteratorBase :
    public PatchControlIterator
{
public:
    ColumnWisePatchIteratorBase(IPatch& patch, int rowDelta) :
        ColumnWisePatchIteratorBase(patch, 0, patch.getWidth() - 1, rowDelta)
    {}

    ColumnWisePatchIteratorBase(IPatch& patch, std::size_t startColumn, std::size_t endColumn, int rowDelta) :
        PatchControlIterator(patch, rowDelta > 0 ? 0 : static_cast<int>(patch.getHeight()) - 1, static_cast<int>(startColumn), 
            std::bind(ColumnWisePatchIteratorBase::moveNext, std::placeholders::_1, std::ref(patch), 
                endColumn, startColumn <= endColumn ? +1 : -1, rowDelta))
    {}

private:
    static void moveNext(PatchControlIterator& it, const IPatch& patch, std::size_t endColumn, int columnDelta, int rowDelta)
    {
        auto nextRow = it.getRow() + rowDelta;
        auto nextColumn = it.getColumn();

        if (rowDelta < 0 && nextRow < 0 ||
            rowDelta > 0 && nextRow >= patch.getHeight())
        {
            // Advance to the next column
            // If that doesn't succeed, just leave the indices out of bounds
            nextColumn += columnDelta;

            if (columnDelta > 0 && nextColumn <= endColumn ||
                columnDelta < 0 && nextColumn >= endColumn)
            {
                nextRow = rowDelta > 0 ? 0 : static_cast<int>(patch.getHeight()) - 1;
            }
        }

        it.set(nextRow, nextColumn);
    }
};

// An iterator traversing a given patch column-wise, iterating over
// one column after the other (which are optionally constrained to [startColumn..endColumn])
class ColumnWisePatchIterator :
    public ColumnWisePatchIteratorBase
{
public:
    ColumnWisePatchIterator(IPatch& patch) :
        ColumnWisePatchIteratorBase(patch, +1)
    {}

    ColumnWisePatchIterator(IPatch& patch, std::size_t startColumn, std::size_t endColumn) :
        ColumnWisePatchIteratorBase(patch, startColumn, endColumn, +1)
    {}
};

// An iterator traversing a given patch column-wise, iterating over
// one column after the other (which are optionally constrained to [startColumn..endColumn])
// with each column traversed backwards, row=[height-1...0]
class ColumnWisePatchReverseIterator :
    public ColumnWisePatchIteratorBase
{
public:
    ColumnWisePatchReverseIterator(IPatch& patch) :
        ColumnWisePatchIteratorBase(patch, -1)
    {}

    ColumnWisePatchReverseIterator(IPatch& patch, std::size_t startColumn, std::size_t endColumn) :
        ColumnWisePatchIteratorBase(patch, startColumn, endColumn, -1)
    {}
};

// An iterator traversing a given patch row-wise, iterating over
// one row after the other (which are optionally constrained to [startRow..endRow])
class RowWisePatchIteratorBase :
    public PatchControlIterator
{
public:
    RowWisePatchIteratorBase(IPatch& patch, int columnDelta) :
        RowWisePatchIteratorBase(patch, 0, patch.getHeight() - 1, columnDelta)
    {}

    RowWisePatchIteratorBase(IPatch& patch, std::size_t startRow, std::size_t endRow, int columnDelta) :
        PatchControlIterator(patch, static_cast<int>(startRow), columnDelta > 0 ? 0 : static_cast<int>(patch.getWidth()) - 1,
            std::bind(RowWisePatchIteratorBase::moveNext, std::placeholders::_1, std::ref(patch), 
                endRow, startRow <= endRow ? +1 : -1, columnDelta))
    {}

private:
    static void moveNext(PatchControlIterator& it, const IPatch& patch, std::size_t endRow, int rowDelta, int columnDelta)
    {
        auto nextColumn = it.getColumn() + columnDelta;
        auto nextRow = it.getRow();

        if (columnDelta > 0 && nextColumn >= patch.getWidth() ||
            columnDelta < 0 && nextColumn < 0)
        {
            // Advance to the next row
            // If that doesn't succeed, just leave the indices out of bounds
            nextRow += rowDelta;

            if (rowDelta > 0 && nextRow <= endRow ||
                rowDelta < 0 && nextRow >= endRow)
            {
                nextColumn = columnDelta > 0 ? 0 : static_cast<int>(patch.getWidth()) - 1;
            }
        }

        it.set(nextRow, nextColumn);
    }
};

// An iterator traversing a given patch row-wise, iterating over
// one row after the other (which are optionally constrained to [startRow..endRow])
// columns are traversed in reverse order, col=[width-1...0]
class RowWisePatchIterator :
    public RowWisePatchIteratorBase
{
public:
    RowWisePatchIterator(IPatch& patch) :
        RowWisePatchIteratorBase(patch, +1)
    {}

    RowWisePatchIterator(IPatch& patch, std::size_t startRow, std::size_t endRow) :
        RowWisePatchIteratorBase(patch, startRow, endRow, +1)
    {}
};

// An iterator traversing a given patch row-wise, iterating over
// one row after the other (which are optionally constrained to [startRow..endRow])
// columns are traversed in reverse order, col=[width-1...0]
class RowWisePatchReverseIterator :
    public RowWisePatchIteratorBase
{
public:
    RowWisePatchReverseIterator(IPatch& patch) :
        RowWisePatchIteratorBase(patch, -1)
    {}

    RowWisePatchReverseIterator(IPatch& patch, std::size_t startRow, std::size_t endRow) :
        RowWisePatchIteratorBase(patch, startRow, endRow, -1)
    {}
};

}
