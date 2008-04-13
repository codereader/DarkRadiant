#ifndef EXPORTER_H_
#define EXPORTER_H_

#include "iselection.h"

/* Exporterclass which will pass every visit-call to a special formatexporter.
 */ 
template<class TExporterFormat>
class CExporter: 
	public SelectionSystem::Visitor
{
public:
	CExporter(std::ofstream& file)
      : m_exporter(file)
    {}
    
    void visit(const scene::INodePtr& node) const {
		m_exporter.visit(node);
    }
    
private:
    mutable TExporterFormat m_exporter;
}; // class CExporter

#endif /*EXPORTER_H_*/
