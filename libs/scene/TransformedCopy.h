#pragma once

namespace scene
{

/**
 * @brief Storage for a value which exists in both a base and a
 * temporarily-transformed state.
 *
 * The base state is typically stored in spawnargs (e.g. the "origin" key),
 * whereas the transformed state is rendered during an ongoing mouse
 * manipulation.
 *
 * @tparam T
 * A copyable value (usually a Vector3 or an angle).
 */
template <typename T> struct TransformedCopy
{
    /// Untransformed value
    T base;

    /// Temporarily-transformed value
    T transformed;

    /// Reset all transformed values to the base values
    void revertTransform()
    {
        transformed = base;
    }
};

} // namespace scene