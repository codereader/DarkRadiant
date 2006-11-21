#ifndef PLUGINSVISITOR_H_
#define PLUGINSVISITOR_H_

#include "iplugin.h"

class PluginsVisitor {
public:
  virtual void visit(IPlugin& plugin) = 0;
};

#endif /*PLUGINSVISITOR_H_*/
