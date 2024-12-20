#include "spreaddatawrapper.h"
#include <QFile>
#include <QIODevice>
#include "dataidentifier.h"
#include "msbase/mesh/deserializecolor.h" 

namespace creative_kernel
{
    SpreadDataWrapper::SpreadDataWrapper(int id, const std::vector<std::string>& data, const QString& dataName) :
        DataSerial(id, dataName)
    {
        m_isCleared = false;
        m_data = data;
		generateIdentifier();
    }

    SpreadDataWrapper::~SpreadDataWrapper() 
    {
        m_storeLaterTimer.stop();
        // if (m_serialized)
        // {
        //     QString fileName = serialFileName();
        //     QFile::remove(fileName);
        // }
    }
	
	bool SpreadDataWrapper::load()
    {
        // if (!m_serialized)
        //     return false;

        QFile file(serialFileName());
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "open spread file failed, file id: " << ID() << ", size: " << m_identifier->size();
            qWarning() << file.errorString();

            return false;
        }
        m_isCleared = false;
        int size = m_identifier->size();

        m_data.clear();
        m_data.resize(size);
        int lineIndex = 0;
        while (!file.atEnd() && lineIndex < size)
        {
            QByteArray array = file.readLine();
            if (array.startsWith(":"))
            {
                QString numberStr = array.mid(1, array.length() - 1);
                int num = numberStr.toInt();
                lineIndex += num;
            }
            else
            {
                array = array.mid(0, array.length() - 1);
                m_data[lineIndex] = std::string(array.data());
                lineIndex++;
            }
        }
        file.close();
        
        return true;
    }

	bool SpreadDataWrapper::store()
    {
        // if (m_serialized)
        //     return false;

         QFile file(serialFileName());
        if (!file.open(QIODevice::WriteOnly))
        {
            return false;
        }
        m_serialized = true;

        QByteArray writedData;
        int emptyCount = 0;
        for (int i = 0, count = m_data.size(); i < count; ++i)
        {
            if (m_data[i].empty())
            {
                if (emptyCount == 0)
                    writedData.append(":");

                emptyCount++;
            }
            else
            {
                if (emptyCount != 0)
                {
                    writedData.append(QString::number(emptyCount));
                    writedData.append("\n");
                    emptyCount = 0;
                }
                writedData.append(m_data[i].data());
                writedData.append("\n");
            }
        }

        if (emptyCount != 0)
        {
            writedData.append(QString::number(emptyCount));
            writedData.append("\n");
        }

        file.write(writedData);
        file.close();
        // m_data.clear();

        return true;
    }

	void SpreadDataWrapper::generateIdentifier()
    {
        m_identifier->generate(m_data);
    }

	void SpreadDataWrapper::clear()
    {
        qWarning() << "remove spread data: id " << ID() << ", size " << m_data.size();
        m_isCleared = true;
        m_data.clear();
    }

	std::vector<std::string> SpreadDataWrapper::data()
    {
        if (m_isCleared)
            load();
        
        //  clearLater(1000);
        return m_data;
    }

	void SpreadDataWrapper::setUsedIndex(const QSet<int>& usedIndexes)
    {
        m_usedIndexes = usedIndexes;
    }
	
    bool SpreadDataWrapper::hasIndex(int index)
    {
        return m_usedIndexes.empty() || m_usedIndexes.contains(index);
    }

	bool SpreadDataWrapper::hasIndexMoreThan(int index)
    {
        if (m_usedIndexes.empty())
            return true;

        for (int usedIndex : m_usedIndexes)
        {
            if (usedIndex >= index)
            {
                return true;
            }
        }
        return false;
    }

    void SpreadDataWrapper::discardIndex(int index)
    {
        if (m_isCleared)
            load();
        
		msbase::change_all_triangles_state(m_data, index);
        generateIdentifier();

        m_serialized = false;
        store();
        // if (!m_isCleared)
        //     storeLater();
    }	

    void SpreadDataWrapper::discardIndexMoreThan(int index)
    {
        if (m_isCleared)
            load();
        
        for (int i = index; i < 16; ++i)
		{
            if (m_usedIndexes.empty() || m_usedIndexes.contains(i))
			    msbase::change_all_triangles_state(m_data, i);
		}
        generateIdentifier();

        m_serialized = false;
        store();
        // if (!m_isCleared)
        //     storeLater();
    }
}