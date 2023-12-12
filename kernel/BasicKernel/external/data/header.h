#ifndef CREATIVE_KERNEL_HEADER_1595470868902_H
#define CREATIVE_KERNEL_HEADER_1595470868902_H
#include "basickernelexport.h"
#include "data/kernelenum.h"

#include <memory>
#include <functional>

#include "trimesh2/TriMesh.h"
#include "trimesh2/TriMesh_algo.h"

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QDebug>
#include <QTime>
namespace creative_kernel
{
	typedef std::shared_ptr<trimesh::TriMesh> TriMeshPtr;

	struct MachineMeta
	{
		QString baseName;
		int extruderCount;
		QString basicMachineName = baseName;//记录该机型是从哪个设备拷贝过来的
		QList<QList<float>> supportExtruderDiameters;
		QList<QString> supportMaterialTypes;
		QList<float> supportMaterialDiameters;
		QList<QString> supportMaterialNames;
		bool isUser = false;

		MachineMeta() {
			baseName = QString("");
			extruderCount = 1;
		}
	};

	struct MachineData
	{
		QList<float> extruderDiameters;
	};

	struct MaterialMeta
	{
		QString name;
		QString type;
		QString brand;
		QList<float> supportDiameters;
		bool isUserDef = false;
		bool isChecked = false;
		bool isVisible = true;
		
		MaterialMeta() {
			name = QString("");
			type = QString("PLA");
			brand = QString("Creality");
			isUserDef = false;
		}

		QString uniqueName() const {
			return QString("%1_%2").arg(name).arg(supportDiameters.at(0));
		}
	};

	struct MaterialData
	{
		QString name;
		QString type;
		QString brand;
		float diameter;
		bool isUserDef = false;
		bool isChecked = false;
		int time = QDateTime::currentMSecsSinceEpoch();
		MaterialData() {
			diameter = 0.4f;
			isUserDef = false;
		}

		QString uniqueName() const {
            QString diameterPrefix = QString("_%1").arg(diameter);
            int index = name.lastIndexOf("_");
            if (index == -1)
                return name + diameterPrefix;
            else
                return name.mid(0, index) + diameterPrefix;
		}
	};
}

#endif // CREATIVE_KERNEL_HEADER_1595470868902_H
