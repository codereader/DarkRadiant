#pragma once

#include <StringSerialisable.h>

#include "ifc/Widget.h"

namespace gtkutil
{

/**
 * \brief
 * ABC for a class which wraps an existing GtkWidget and implements
 * StringSerialisable to read/write the data from that widget.
 */
class SerialisableWidgetWrapper
: public StringSerialisable,
  public Widget
{ 
   // The widget
   GtkWidget* _widget;

protected:

   /* Widget implementation */
   GtkWidget* _getWidget() const
   {
      return _widget;
   }

public:

   /**
    * \brief
    * Construct a SerialisableWidgetWrapper to wrap and serialise the given
    * GtkWidget.
    */
   SerialisableWidgetWrapper(GtkWidget* w)
   : _widget(w)
   { }
};

// Shared pointer typedef
typedef 
boost::shared_ptr<SerialisableWidgetWrapper> SerialisableWidgetWrapperPtr;

/**
 * \brief
 * Serialisable GtkAdjustment class.
 *
 * A GtkAdjustment is not a GtkWidget but a GtkObject, so this is not a subclass
 * of SerialisableWidgetWrapper.
 */
class SerialisableAdjustment
: public StringSerialisable
{
   // The adjustment
   GtkObject* _adjustment;

public:

   // Main constructor
   SerialisableAdjustment(GtkObject* adj);

   /* StringSerialisable implementation */
   void importFromString(const std::string& str);
   std::string exportToString() const;
};

/*
 * Objects which implement StringSerialisable and wrap a GtkWidget.
 */

class SerialisableTextEntry
: public SerialisableWidgetWrapper
{
public:

   // Main constructor
   SerialisableTextEntry(GtkWidget* w);

   /* StringSerialisable implementation */
   void importFromString(const std::string& str);
   std::string exportToString() const;
};

class SerialisableSpinButton
: public SerialisableWidgetWrapper
{
public:

   // Main constructor
   SerialisableSpinButton(GtkWidget* w);

   /* StringSerialisable implementation */
   void importFromString(const std::string& str);
   std::string exportToString() const;
};

class SerialisableScaleWidget
: public SerialisableWidgetWrapper
{
public:

   // Main constructor
   SerialisableScaleWidget(GtkWidget*);

   /* StringSerialisable implementation */
   void importFromString(const std::string& str);
   std::string exportToString() const;
};

class SerialisableToggleButton
: public SerialisableWidgetWrapper
{
public:

   // Main constructor
   SerialisableToggleButton(GtkWidget* w);

   /* StringSerialisable implementation */
   void importFromString(const std::string& str);
   std::string exportToString() const;
};


/**
 * \brief
 * StringSerialisable combo box which saves the selected value as a numeric
 * index.
 */
class SerialisableComboBox_Index
: public SerialisableWidgetWrapper
{
public:

   /* Main constructor */
   SerialisableComboBox_Index();

   /* StringSerialisable implementation */
   void importFromString(const std::string& str);
   std::string exportToString() const;
};

/**
 * \brief
 * Serialisable combo box which saves the selected value as a text string.
 */
class SerialisableComboBox_Text
: public SerialisableWidgetWrapper
{
public:

   /* Main constructor */
   SerialisableComboBox_Text();

   /* StringSerialisable implementation */
   void importFromString(const std::string& str);
   std::string exportToString() const;
};

}

