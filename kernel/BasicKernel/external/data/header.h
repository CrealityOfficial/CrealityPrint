#ifndef CREATIVE_KERNEL_HEADER_1595470868902_H
#define CREATIVE_KERNEL_HEADER_1595470868902_H
#include "basickernelexport.h"
#include "data/kernelenum.h"

#include <memory>
#include <functional>

#include "trimesh2/TriMesh.h"
#include "trimesh2/TriMesh_algo.h"
#include "msbase/data/quaterniond.h"

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QDebug>

namespace creative_kernel
{
	typedef std::shared_ptr<trimesh::TriMesh> TriMeshPtr;

	struct MachineData
	{
		QString baseName;
		QString codeName;
		int extruderCount = 1;
		QList<float> extruderDiameters;
		QList<QString> supportMaterialTypes;
		QList<float> supportMaterialDiameters;
		QList<QString> supportMaterialNames;
		QList<QString> top_material;
		bool isUser = false;
		bool is_bbl_printer = false;
		bool is_import = false;
		QString inherits_from;
		QString preferredProcess;
		QString uniqueName() const {
			QString ret = codeName;
			for (const auto& diameter : extruderDiameters)
			{
				ret += QString("-%1").arg(diameter);
			}
			return ret;
		}
		QString uniqueShowName() const {
			QString ret = baseName;
			for (const auto& diameter : extruderDiameters)
			{
				ret += QString("-%1").arg(diameter);
			}
			return ret;
		}
	};

	struct MaterialData
	{
		QString name;
		QString type;
		QString brand;
		float diameter;
		int rank = 0;
		bool isUserDef = false;
		bool isMachineDef = false;
		bool isChecked = false;
		bool isVisible = true;
		QString id;

		MaterialData() {
			diameter = 1.75f;
		}

		QString uniqueName() const {
			return QString("%1-%2").arg(name).arg(diameter);
		}
	};
}

#endif // CREATIVE_KERNEL_HEADER_1595470868902_H
