#ifndef QTUSER_CORE_CSVWRITER_1596547558084_H
#define QTUSER_CORE_CSVWRITER_1596547558084_H
#include "qtusercore/qtusercoreexport.h"
#include "qtusercore/module/timestamp.h"
#include <vector>

namespace qtuser_core
{
	class QTUSER_CORE_API CSVWriter : public QObject
	{
		Q_OBJECT
	public:
		CSVWriter(QObject* parent = nullptr);
		virtual ~CSVWriter();

		void clear();
		void pushHead(const std::string& head);
		void pushData(double value);
		void tickStart();
		void tickEnd();
		void start(timestamp* time = 0);
		void tick(bool delta = false);
		void output(const std::string& file);
	protected:
		std::vector<std::string> m_headers;
		std::vector<double> m_values;

		timestamp m_start_time;
		timestamp m_flag_time;

		int m_index;
	};
}
#endif // QTUSER_CORE_CSVWRITER_1596547558084_H