#ifndef SLICE_RESULT_1590033290815_H
#define SLICE_RESULT_1590033290815_H
#include "crslice/interface.h"
#include "ccglobal/tracer.h"
#include "trimesh2/Box.h"
#include <vector>
#include <list>
#include <memory>

namespace gcode
{
    class CRSLICE_API SliceResult
    {
    public:
        SliceResult();
        ~SliceResult();

        const std::string& prefixCode();
        const std::vector<std::string>& layerCode();
        const std::string& tailCode();
        const std::string& layer(int index);

        trimesh::box3 inputBox;

        void clear();
        bool load(const std::string& fileName, ccglobal::Tracer* tracer = nullptr);
        bool load_pressureEquity(const std::string& fileName, ccglobal::Tracer* tracer = nullptr);
        void setFileName(const std::string& fileName);
        bool set_data_gcodelayer(int layer,const std::string& gcodelayer);//set a layer data
        bool set_gcodeprefix(const std::string& gcodeprefix);//set gcodeprefix
		std::string fileName();

		std::string sliceName();
        void setSliceName(const std::string& sliceName);
		std::vector<std::vector<unsigned char>> previewsData;
    private:
        std::string m_emptyString;
        std::vector<std::string> m_data_gcodelayer;//GCodeLayer
		std::vector<std::string> m_data_gcodeprefix; //GCodePrefix.
		std::vector<std::string> m_data_gcodetail; //GCodetail.

		std::string m_fileName;
		std::string m_sliceName;
    };
}

typedef CRSLICE_API std::shared_ptr<gcode::SliceResult> SliceResultPointer;
#endif // SLICE_RESULT_1590033290815_H
