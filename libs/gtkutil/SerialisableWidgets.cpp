#include "SerialisableWidgets.h"

#include "string/string.h"

#include <gtk/gtk.h>
#include <boost/lexical_cast.hpp>
#include <iostream>

namespace gtkutil
{

// Adjustment

SerialisableAdjustment::SerialisableAdjustment(GtkObject* adj)
: _adjustment(adj)
{ }

void SerialisableAdjustment::importFromString(const std::string& str)
{
    gtk_adjustment_set_value(GTK_ADJUSTMENT(_adjustment), strToFloat(str));
}

std::string SerialisableAdjustment::exportToString() const
{
   return floatToStr(gtk_adjustment_get_value(GTK_ADJUSTMENT(_adjustment)));
}

// Text entry

SerialisableTextEntry::SerialisableTextEntry(GtkWidget* w)
: SerialisableWidgetWrapper(w)
{ }

void SerialisableTextEntry::importFromString(const std::string& str)
{
   gtk_entry_set_text(GTK_ENTRY(_getWidget()), str.c_str());
}

std::string SerialisableTextEntry::exportToString() const
{
   return std::string(gtk_entry_get_text(GTK_ENTRY(_getWidget())));
}

// Spin button

SerialisableSpinButton::SerialisableSpinButton(GtkWidget* w)
: SerialisableWidgetWrapper(w)
{ }

void SerialisableSpinButton::importFromString(const std::string& str)
{
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(_getWidget()), strToFloat(str));
}

std::string SerialisableSpinButton::exportToString() const
{
   return floatToStr(gtk_spin_button_get_value(GTK_SPIN_BUTTON(_getWidget())));
}

// Scale widget

SerialisableScaleWidget::SerialisableScaleWidget(GtkWidget* w)
: SerialisableWidgetWrapper(w)
{ }

void SerialisableScaleWidget::importFromString(const std::string& str)
{
   gtk_range_set_value(GTK_RANGE(_getWidget()), strToFloat(str));
}

std::string SerialisableScaleWidget::exportToString() const
{
   return floatToStr(gtk_range_get_value(GTK_RANGE(_getWidget())));
}

// Toggle button

SerialisableToggleButton::SerialisableToggleButton(GtkWidget* w)
: SerialisableWidgetWrapper(w)
{ }

void SerialisableToggleButton::importFromString(const std::string& str)
{
   gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(_getWidget()), 
      str == "1" ? TRUE : FALSE
   );
}

std::string SerialisableToggleButton::exportToString() const
{
   if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_getWidget())) == TRUE)
      return "1";
   else 
      return "0";
}


// SerialisableComboBox_Index

SerialisableComboBox_Index::SerialisableComboBox_Index()
: SerialisableWidgetWrapper(gtk_combo_box_new_text())
{ }

void SerialisableComboBox_Index::importFromString(const std::string& str)
{
   int activeId = boost::lexical_cast<int>(str);
   gtk_combo_box_set_active(GTK_COMBO_BOX(_getWidget()), activeId);

   int newId = gtk_combo_box_get_active(GTK_COMBO_BOX(_getWidget()));
   if (activeId != newId)
   {
      std::cerr << "SerialisableComboBox_Index::importFromString(): "
                << "warning: requested index " << activeId 
                << " was not set, current index is " << newId << std::endl;
   }
}

std::string SerialisableComboBox_Index::exportToString() const
{
   return boost::lexical_cast<std::string>(
      gtk_combo_box_get_active(GTK_COMBO_BOX(_getWidget()))
   );
}

// SerialisableComboBox_Text

SerialisableComboBox_Text::SerialisableComboBox_Text()
: SerialisableWidgetWrapper(gtk_combo_box_new_text())
{ }

void SerialisableComboBox_Text::importFromString(const std::string& str)
{
   int index = -1;

   // Find the index of the given text
   GtkTreeIter iter;
   GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(_getWidget()));
   for (gboolean validIter = gtk_tree_model_get_iter_first(model, &iter);
        validIter == TRUE;
        validIter = gtk_tree_model_iter_next(model, &iter))
   {
      // Get the string value
      gchar* treeVal;
      gtk_tree_model_get(model, &iter, 0, &treeVal, -1);

      // Check if this is the right string
      index++;
      if (std::string(treeVal) == str)
      {
         break;
      }
   };

   if (index == -1)
   {
      std::cerr << "SerialisableComboBox_Text::importFromString(): "
                << "unable to find string '" << str << "' in combo box"
                << std::endl;
   }
   else
   {
      gtk_combo_box_set_active(GTK_COMBO_BOX(_getWidget()), index);
   }
}

std::string SerialisableComboBox_Text::exportToString() const
{
   return gtk_combo_box_get_active_text(
      GTK_COMBO_BOX(_getWidget())
   );
}

}
