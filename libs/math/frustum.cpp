#include "frustum.h"

void Frustum::normalisePlanes()
{
    left = left.getNormalised();
    right = right.getNormalised();
    top = top.getNormalised();
    bottom = bottom.getNormalised();
    back = back.getNormalised();
    front = front.getNormalised();
}

