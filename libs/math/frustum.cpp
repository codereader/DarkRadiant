#include "frustum.h"

// Normalise all planes in frustum
void Frustum::normalisePlanes()
{
    left = left.getNormalised();
    right = right.getNormalised();
    top = top.getNormalised();
    bottom = bottom.getNormalised();
    back = back.getNormalised();
    front = front.getNormalised();
}

// Get a projection matrix from the frustum
Matrix4 Frustum::getProjectionMatrix() const
{
    return Matrix4(
        // col 1
        (right.a - left.a) / 2,
        (top.a - bottom.a) / 2,
        (back.a - front.a) / 2,
        right.a - (right.a - left.a) / 2,
        // col 2
        (right.b - left.b) / 2,
        (top.b - bottom.b) / 2,
        (back.b - front.b) / 2,
        right.b - (right.b - left.b) / 2,
        // col 3
        (right.c - left.c) / 2,
        (top.c - bottom.c) / 2,
        (back.c - front.c) / 2,
        right.c - (right.c - left.c) / 2,
        // col 4
        (right.d - left.d) / 2,
        (top.d - bottom.d) / 2,
        (back.d - front.d) / 2,
        right.d - (right.d - left.d) / 2
    );
}

