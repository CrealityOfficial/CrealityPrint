#ifndef _FDMSUPPORTHANDLER_1595487718947_H
#define _FDMSUPPORTHANDLER_1595487718947_H
#include <QtCore/QObject>

namespace qtuser_3d
{
	class FacePicker;
}

namespace creative_kernel
{
	class FDMSupport;
	class FDMSupportGroup;
}

class FDMSupportHandler : public QObject
{
	Q_OBJECT
public:
	FDMSupportHandler(QObject* parent = nullptr);
	virtual ~FDMSupportHandler();

	void setPickSource(qtuser_3d::FacePicker* picker);
	void clear();

	void reset();
	creative_kernel::FDMSupport* hover(const QPoint& point, QVector3D& position);
	creative_kernel::FDMSupport* check(const QPoint& point, QVector3D& position);
protected:
	creative_kernel::FDMSupportGroup* face2SupportGroup(int faceID);
protected:
	qtuser_3d::FacePicker* m_pickSource;

	creative_kernel::FDMSupport* m_hoverSupport;
};

#endif // _FDMSUPPORTHANDLER_1595487718947_H