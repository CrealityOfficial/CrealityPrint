#include "cx3dnewbuild.h"
#include "interface/spaceinterface.h"
#include <QFileInfo>
#include <interface/projectinterface.h>
namespace creative_kernel
{
	CX3DNewBuild::CX3DNewBuild(QObject* parent)
		:QObject(parent)
	{
	}

	CX3DNewBuild::~CX3DNewBuild()
	{
	}


	QString CX3DNewBuild::filter()
	{
		QString str = tr("Project File (") + QString("*.cxprj)");
		return str;
	}

	void CX3DNewBuild::handle(const QString& fileName)
	{
		//�򿪹����ļ�ʱ����Ҫ��ճ����������ģ��
		qDebug() << "fileName =" << fileName;
		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{

		}
		file.close();
		//����·���͹�����
		QFileInfo  info(fileName);
		creative_kernel::projectUI()->setProjectPath(fileName);
		creative_kernel::projectUI()->setProjectName(info.fileName());
		//���ƽ̨�Լ���������
		creative_kernel::projectUI()->clearSecen();
		//uninitializeSpace();
	}

	void CX3DNewBuild::handle(const QStringList& fileNames)
	{

	}
}