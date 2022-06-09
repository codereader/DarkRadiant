#pragma once

#include "iselectiontest.h"
#include "ObservedSelectable.h"
#include "math/AABB.h"
#include "math/Line.h"

namespace selection
{

/**
 * Selection-test and transformation helper for drag-resizable objects. 
 * This is used by PatchNodes, LightNodes and SpeakerNodes.
 */
class DragPlanes
{
private:
    ObservedSelectable _selectableRight; // +x
    ObservedSelectable _selectableLeft; // -x
    ObservedSelectable _selectableFront; // +y
    ObservedSelectable _selectableBack; // -y
    ObservedSelectable _selectableTop; // +z
    ObservedSelectable _selectableBottom; // -z

public:
    AABB m_bounds;

    DragPlanes(const SelectionChangedSlot& onchanged) :
        _selectableRight(onchanged),
        _selectableLeft(onchanged),
        _selectableFront(onchanged),
        _selectableBack(onchanged),
        _selectableTop(onchanged),
        _selectableBottom(onchanged)
    {}

    // Returns true if any of the 6 planes is selected
    bool isSelected() const
    {
        return _selectableRight.isSelected() || _selectableLeft.isSelected() ||
               _selectableFront.isSelected() || _selectableBack.isSelected() || 
               _selectableTop.isSelected() || _selectableBottom.isSelected();
    }

    void setSelected(bool selected)
    {
        _selectableRight.setSelected(selected);
        _selectableLeft.setSelected(selected);
        _selectableFront.setSelected(selected);
        _selectableBack.setSelected(selected);
        _selectableTop.setSelected(selected);
        _selectableBottom.setSelected(selected);
    }

    // greebo: Test-select each of the 6 planes and add suitable ones to the Selector
    // for further consideration. The basic idea of this method is to do a simple
    // "is this test position in front of any of these 6 planes"-test by calculating
    // dot products with all corresponding 4 corner vertices of each plane. For a plane
    // to pass the test it's required that all its 4 corner vertices need to pass.
    // Everything passed in here should be specified in local coordinates, any 
    // local2world transformation needs to be set up in the SelectionTest beforehand.
    void selectPlanes(const AABB& aabb, 
                      Selector& selector, 
                      SelectionTest& test, 
                      const PlaneCallback& selectedPlaneCallback)
    {
        // Provided that the object's local2World matrix has been fed into the SelectionTest
        // the getNear() and getFar() methods will return local coordinates.
        Line line(test.getNear(), test.getFar());

        // Calculate the corners (local coords)
        Vector3 corners[8];
        aabb.getCorners(corners);

        Plane3 planes[6];
        aabb.getPlanes(planes);

        // Next calculate the vectors for the upcoming dot-product
        // Take the closest point of the selection ray and calc the distance vector
        for (Vector3* i = corners; i != corners + 8; ++i)
        {
            *i = line.getClosestPoint(*i) - (*i);
        }

        if (planes[0].normal().dot(corners[1]) > 0 && planes[0].normal().dot(corners[2]) > 0 &&
            planes[0].normal().dot(corners[5]) > 0 && planes[0].normal().dot(corners[6]) > 0)
        {
            selector.addWithNullIntersection(_selectableRight);
            selectedPlaneCallback(planes[0]);
            //rMessage() << "right\n";
        }

        if (planes[1].normal().dot(corners[0]) > 0 && planes[1].normal().dot(corners[3]) > 0 && 
            planes[1].normal().dot(corners[4]) > 0 && planes[1].normal().dot(corners[7]) > 0)
        {
            selector.addWithNullIntersection(_selectableLeft);
            selectedPlaneCallback(planes[1]);
            //rMessage() << "left\n";
        }

        if (planes[2].normal().dot(corners[0]) > 0 && planes[2].normal().dot(corners[1]) > 0 && 
            planes[2].normal().dot(corners[4]) > 0 && planes[2].normal().dot(corners[5]) > 0)
        {
            selector.addWithNullIntersection(_selectableFront);
            selectedPlaneCallback(planes[2]);
            //rMessage() << "front\n";
        }

        if (planes[3].normal().dot(corners[2]) > 0 && planes[3].normal().dot(corners[3]) > 0 && 
            planes[3].normal().dot(corners[6]) > 0 && planes[3].normal().dot(corners[7]) > 0)
        {
            selector.addWithNullIntersection(_selectableBack);
            selectedPlaneCallback(planes[3]);
            //rMessage() << "back\n";
        }

        if (planes[4].normal().dot(corners[0]) > 0 && planes[4].normal().dot(corners[1]) > 0 && 
            planes[4].normal().dot(corners[2]) > 0 && planes[4].normal().dot(corners[3]) > 0)
        {
            selector.addWithNullIntersection(_selectableTop);
            selectedPlaneCallback(planes[4]);
            //rMessage() << "top\n";
        }

        if (planes[5].normal().dot(corners[4]) > 0 && planes[5].normal().dot(corners[5]) > 0 && 
            planes[5].normal().dot(corners[6]) > 0 && planes[5].normal().dot(corners[7]) > 0)
        {
            selector.addWithNullIntersection(_selectableBottom);
            selectedPlaneCallback(planes[5]);
            //rMessage() << "bottom\n";
        }

        m_bounds = aabb;
    }

    void selectReversedPlanes(const AABB& aabb, Selector& selector, const SelectedPlanes& selectedPlanes)
    {
        Plane3 planes[6];
        aabb.getPlanes(planes);

        if (selectedPlanes.contains(-planes[0]))
        {
            selector.addWithNullIntersection(_selectableRight);
        }

        if (selectedPlanes.contains(-planes[1]))
        {
            selector.addWithNullIntersection(_selectableLeft);
        }

        if (selectedPlanes.contains(-planes[2]))
        {
            selector.addWithNullIntersection(_selectableFront);
        }

        if (selectedPlanes.contains(-planes[3]))
        {
            selector.addWithNullIntersection(_selectableBack);
        }

        if (selectedPlanes.contains(-planes[4]))
        {
            selector.addWithNullIntersection(_selectableTop);
        }

        if (selectedPlanes.contains(-planes[5]))
        {
            selector.addWithNullIntersection(_selectableBottom);
        }
    }

    // greebo: This calculates a new AABB for the given translation. Everything used in here is in local coordinates:
    // The m_bounds member (which has to be set beforehand) as well as the translation vector. 
    // Based on the selection status of the 6 planes a new min/max vector pair is calculated 
    // and the resulting AABB is returned.
    AABB evaluateResize(const Vector3& translation) const
    {
        Vector3 min = m_bounds.getOrigin() - m_bounds.getExtents();
        Vector3 max = m_bounds.getOrigin() + m_bounds.getExtents();

        // Handle x translation
        if (m_bounds.extents.x() != 0)
        {
            if (_selectableRight.isSelected())
            {
                max.x() += translation.x();
                //rMessage() << "moving right\n";
            }

            if (_selectableLeft.isSelected())
            {
                min.x() += translation.x();
                //rMessage() << "moving left\n";
            }
        }

        // Handle y translation
        if (m_bounds.extents.y() != 0)
        {
            if (_selectableFront.isSelected())
            {
                max.y() += translation.y();
                //rMessage() << "moving front\n";
            }

            if (_selectableBack.isSelected())
            {
                min.y() += translation.y();
                //rMessage() << "moving back\n";
            }
        }

        // Handle z translation
        if (m_bounds.extents.z() != 0)
        {
            if (_selectableTop.isSelected())
            {
                max.z() += translation.z();
                //rMessage() << "moving top\n";
            }

            if(_selectableBottom.isSelected())
            {
                min.z() += translation.z();
                //rMessage() << "moving bottom\n";
            }
        }

        return AABB::createFromMinMax(min, max);
    }

    // greebo: Evaluates the given translation for the rotated m_bounds and return a new one.
    // Translation is in world coordinates, whereas the (previously set) m_bounds AABB is in local coords.
    // The given rotation matrix is used to transform the incoming translation. Only those planes which
    // have been selected beforehand will be translated. The m_bounds member needs to be set beforehand. 
    // As PatchNodes and SpeakerNodes are quasi non-rotated objects, this code applies to LightNodes only.
    AABB evaluateResize(const Vector3& translation, const Matrix4& rotation) const
    {
        //rMessage() << "Translation: " << translation << ", to local: " << translationToLocal(translation, rotation) << std::endl;

        // Convert the translation to local coords and calculate a resized AABB (in local coords).
        AABB aabb(evaluateResize(translationToLocal(translation, rotation)));

        // The origin is moved by this operation as well:
        // Rotate the diff vector between oldLocalOrigin and newLocalOrigin and apply it to the AABB
        aabb.origin = m_bounds.origin + translationFromLocal(aabb.origin - m_bounds.origin, rotation);

        return aabb;
    }

    // greebo: This is used by PatchNodes to calculate a generic transformation matrix from the given
    // drag-manipulation, which is then applied to the whole object.
    Matrix4 evaluateTransform(const Vector3& translation) const
    {
        AABB aabb(evaluateResize(translation));

        Vector3 scale(
            m_bounds.extents[0] != 0 ? aabb.extents[0] / m_bounds.extents[0] : 1,
            m_bounds.extents[1] != 0 ? aabb.extents[1] / m_bounds.extents[1] : 1,
            m_bounds.extents[2] != 0 ? aabb.extents[2] / m_bounds.extents[2] : 1
        );

        Matrix4 matrix = Matrix4::getTranslation(aabb.origin - m_bounds.origin);
        matrix.scaleBy(scale, m_bounds.origin);

        return matrix;
    }

private:
    // local must be a pure rotation
    Vector3 translationToLocal(const Vector3& translation, const Matrix4& local) const
    {
        return local.getTransposed().getTranslatedBy(translation).getMultipliedBy(local).translation();
    }

    // local must be a pure rotation
    Vector3 translationFromLocal(const Vector3& translation, const Matrix4& local) const
    {
        return local.getTranslatedBy(translation).getMultipliedBy(local.getTransposed()).translation();
    }
};

} // namespace
