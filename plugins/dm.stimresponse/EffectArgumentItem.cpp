#include "EffectArgumentItem.h"

#include <gtk/gtk.h>
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"

EffectArgumentItem::EffectArgumentItem(
		ResponseEffect::Argument& arg, 
		GtkTooltips* tooltips) :
	_arg(arg),
	_tooltips(tooltips)
{
	// Pack the label into a eventbox
	_labelBox = gtk_event_box_new();
	GtkWidget* label = gtkutil::LeftAlignedLabel(_arg.title + ":");
	gtk_container_add(GTK_CONTAINER(_labelBox), label);
	
	gtk_tooltips_set_tip(_tooltips, _labelBox, arg.desc.c_str(), "");
	
	// Pack the description widget into a eventbox
	_descBox = gtk_event_box_new();
	GtkWidget* descLabel = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(descLabel), "<b>?</b>");
	gtk_container_add(GTK_CONTAINER(_descBox), descLabel);
	
	gtk_tooltips_set_tip(_tooltips, _descBox, arg.desc.c_str(), "");
}

// Retrieve the label widget
GtkWidget* EffectArgumentItem::getLabelWidget() {
	return _labelBox;
}

GtkWidget* EffectArgumentItem::getHelpWidget() {
	return _descBox;
}

void EffectArgumentItem::save() {
	// Save the value to the effect 
	_arg.value = getValue();
}

// StringArgument
StringArgument::StringArgument(
		ResponseEffect::Argument& arg, 
		GtkTooltips* tooltips) :
	EffectArgumentItem(arg, tooltips)
{
	_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(_entry), arg.value.c_str()); 
}

GtkWidget* StringArgument::getEditWidget() {
	return _entry;
}

std::string StringArgument::getValue() {
	return gtk_entry_get_text(GTK_ENTRY(_entry));
}

// Entity Argument
EntityArgument::EntityArgument(
		ResponseEffect::Argument& arg, 
		GtkTooltips* tooltips) :
	EffectArgumentItem(arg, tooltips)
{}

std::string EntityArgument::getValue() {
	return "TestEntity";
}

GtkWidget* EntityArgument::getEditWidget() {
	return gtk_entry_new();
}
