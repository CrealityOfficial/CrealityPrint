#include "placeonprinterbed.h"

#include "interface/selectorinterface.h"
#include "interface/modelinterface.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
    PlaceOnPrinterBed::PlaceOnPrinterBed(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Place On PrinterBed");
        m_actionNameWithout = "Place On PrinterBed";

        addUIVisualTracer(this);
    }

    PlaceOnPrinterBed::~PlaceOnPrinterBed()
    {
    }

    void PlaceOnPrinterBed::execute()
    {
        bottomSelectionModels(true);
    }

    bool PlaceOnPrinterBed::enabled()
    {
        return selectionms().size() > 0;
    }

    void PlaceOnPrinterBed::onThemeChanged(ThemeCategory category)
    {

    }

    void PlaceOnPrinterBed::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Place On PrinterBed");
    }
}
