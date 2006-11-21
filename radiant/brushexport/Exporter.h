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
    CExporter(TextFileOutputStream& file)
      : m_exporter(file)
    {}
    
	virtual ~CExporter(void) {}
    
    void visit(scene::Instance& instance) const {
		m_exporter.visit(instance);
    }
    
private:
    mutable TExporterFormat m_exporter;
}; // class CExporter

#endif /*EXPORTER_H_*/
