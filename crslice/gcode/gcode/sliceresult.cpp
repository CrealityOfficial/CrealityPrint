#include "crslice/gcode/sliceresult.h"
#include "crslice/gcode/parasegcode.h"

namespace gcode
{
    SliceResult::SliceResult() 
    {

    }

    SliceResult::~SliceResult()
    {
    }

    const std::string& SliceResult::prefixCode()
    {
        if (m_data_gcodeprefix.size() > 0)
            return *m_data_gcodeprefix.begin();
        return m_emptyString;
    }

    const std::vector<std::string>& SliceResult::layerCode()
    {
        return m_data_gcodelayer;
    }

    const std::string& SliceResult::layer(int index)
    {
        if (index < 0 || index >= (int)m_data_gcodelayer.size())
            return m_emptyString;

        return m_data_gcodelayer.at(index);
    }

    const std::string& SliceResult::tailCode()
    {
        if (m_data_gcodetail.size() > 0)
            return *(m_data_gcodetail.begin());
        return m_emptyString;
    }

    void SliceResult::clear()
    {
        m_data_gcodelayer.clear();
        m_data_gcodeprefix.clear();
    }

	unsigned long long get_file_size(std::ifstream& fin)
	{
		fin.seekg(0, fin.end);
		auto fos = fin.tellg();
		unsigned long long filesize = fos;
		fin.seekg(0);
		return filesize;
	}

	bool SliceResult::load(const std::string& fileName, ccglobal::Tracer* tracer)
    {
		m_fileName = fileName;
		m_data_gcodelayer.clear();
		m_data_gcodeprefix.clear();

		std::ifstream  fileInfo(fileName);
		if (!fileInfo.is_open())
		{
			return false;
		}

		unsigned long long fileSize = get_file_size(fileInfo);
		std::string value = "";
		bool headend = false;
		unsigned long long int readBytes = 0;
		std::string temp;
		
		while (std::getline(fileInfo, temp))
		{			
			value += (temp + "\n");
			readBytes += temp.size();
			if (tracer && (readBytes % 20000) == 0)
			{
				float p = (float)((double)readBytes / (double)fileSize);
				tracer->progress(p);

				if (tracer->interrupt())
				{
					tracer->failed("cxLoadGCode load interrupt ");
					m_data_gcodelayer.clear();
					break;
				}
			}

			if (temp.find(";LAYER_COUNT")!= std::string::npos)
			{
				if (!headend)
				{
					m_data_gcodeprefix.push_back(value);
				}
				else
				{
					//针对逐个打印 增加抬升代码
					std::string str = m_data_gcodelayer.back();
					str += value;
					m_data_gcodelayer.back() = str;
				}

				//layerString.append(temp);
				headend = true;
				value.clear();
				continue;
			}
			if (!headend)
			{
				continue;
			}
			if (temp.find(";TIME_ELAPSED:") != std::string::npos)
			{
				m_data_gcodelayer.push_back(value);
				value.clear();
				continue;
			}
		}
		m_data_gcodetail.push_back(value);
		fileInfo.close();
		return true;
    }
	bool SliceResult::load_pressureEquity(const std::string& fileName, ccglobal::Tracer* tracer)
	{
		m_fileName = fileName;
		m_data_gcodelayer.clear();
		m_data_gcodeprefix.clear();

		std::ifstream  fileInfo(fileName);
		if (!fileInfo.is_open())
		{
			return false;
		}

		unsigned long long fileSize = get_file_size(fileInfo);
		std::string value = "";
		bool headend = false;
		unsigned long long int readBytes = 0;
		std::string temp;

		while (std::getline(fileInfo, temp))
		{
			std::vector<std::string> oneLS;
			Stringsplit(temp, ':', oneLS);
			if (oneLS.size() == 2 && oneLS.at(0) == ";TYPE")
			{
				if (oneLS.at(1) == "WALL-INNER" || oneLS.at(1) == "WALL-OUTER")
				{
					value += (";_EXTRUSION_ROLE:1\n");
				}
				if (oneLS.at(1) == "SKIN")
				{
					value += (";_EXTRUSION_ROLE:2\n");
				}
				if (oneLS.at(1) == "FILL")
				{
					value += (";_EXTRUSION_ROLE:3\n");
				}
				if (oneLS.at(1) == "SKIRT")
				{
					value += (";_EXTRUSION_ROLE:4\n");
				}
				if (oneLS.at(1) == "SUPPORT-INTERFACE" || oneLS.at(1) == "SUPPORT")
				{
					value += (";_EXTRUSION_ROLE:5\n");
				}
			}

			if (temp == ";;retract;;" || temp == ";;retract move;;")
			{
				value += (";_EXTRUSION_ROLE:0\n");
			}

			value += (temp + "\n");
			readBytes += temp.size();
			if (tracer && (readBytes % 20000) == 0)
			{
				float p = (float)((double)readBytes / (double)fileSize);
				tracer->progress(p);

				if (tracer->interrupt())
				{
					tracer->failed("cxLoadGCode load interrupt ");
					m_data_gcodelayer.clear();
					break;
				}
			}

			if (temp.find(";LAYER_COUNT") != std::string::npos)
			{
				if (!headend)
				{
					m_data_gcodeprefix.push_back(value);
				}
				else
				{
					//针对逐个打印 增加抬升代码
					std::string str = m_data_gcodelayer.back();
					str += value;
					m_data_gcodelayer.back() = str;
				}

				//layerString.append(temp);
				headend = true;
				value.clear();
				continue;
			}
			if (!headend)
			{
				continue;
			}
			if (temp.find(";TIME_ELAPSED:") != std::string::npos)
			{
				m_data_gcodelayer.push_back(value);
				value.clear();
				continue;
			}
		}
		m_data_gcodetail.push_back(value);
		fileInfo.close();
		return true;
	}
	void SliceResult::setFileName(const std::string& fileName)
	{
		m_fileName = fileName;
	}

	bool SliceResult::set_data_gcodelayer(int layer, const std::string& gcodelayer)//set a layer data
	{
		int count = layer - m_data_gcodelayer.size();
		if (count >= 0)
		{
			for (int i = 0; i < count+1; i++)
			{
				m_data_gcodelayer.push_back("");
			}
		}

		m_data_gcodelayer[layer] = gcodelayer;
		return true;
	}

	bool SliceResult::set_gcodeprefix(const std::string& gcodeprefix)//set gcodeprefix
	{
		if (m_data_gcodeprefix.empty())
		{
			m_data_gcodeprefix.push_back(gcodeprefix);
		}
		else
			m_data_gcodeprefix[0] = gcodeprefix;
		return true;
	}


	std::string SliceResult::fileName()
    {
        return m_fileName;
    }

	std::string SliceResult::sliceName()
    {
        return m_sliceName;
    }

    void SliceResult::setSliceName(const std::string& _sliceName)
    {
        m_sliceName = _sliceName;
    }
}
