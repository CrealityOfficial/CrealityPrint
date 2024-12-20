#ifndef _ABSTRACT_MEASURE_OBJ_H__
#define _ABSTRACT_MEASURE_OBJ_H__

#include <QVector3D>
#include <QMatrix4x4>
#include <qobject.h>


class AbstractMeasureObj
{
public:
	enum class MeasureInputType
	{
		HOVER = 0,
		CLICK = 1,
		
	};

	enum class MeasureResultType
	{
		MID = 0,
		FINAL = 1,

	};

public:
	AbstractMeasureObj(QObject* object);
	virtual ~AbstractMeasureObj();

	int measureType() const;

	QString measureValue() const;

	virtual int reset() = 0;

	virtual int addPoint(const QVector3D& position, const QVector3D& normal, const QMatrix4x4& m, MeasureInputType type) = 0;

	virtual int updateShowUI() = 0;

	virtual bool complete() = 0;

protected:
	int m_measureType;    // 0： 无； 1：点到点； 2：点到面； 3：面到面
	
	QString m_measureValue;     // 最终结果的字符串

protected:
	// UI 相关
	QObject* m_attachUI;

};


#endif
