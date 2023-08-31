#ifndef CREATIVE_KERNEL_FDMSUPPORTGROUP_1595470832654_H
#define CREATIVE_KERNEL_FDMSUPPORTGROUP_1595470832654_H
#include "data/header.h"
#include "qtuser3d/module/node3d.h"
#include "data/fdmsupportparam.h"

namespace qtuser_3d
{
	class SupportChunkGroupEntity;
}

namespace creative_kernel
{
	class FDMSupport;
	class BASIC_KERNEL_API FDMSupportGroup : public qtuser_3d::Node3D
	{
		Q_OBJECT
	public:
		FDMSupportGroup(qtuser_3d::Node3D* parent = nullptr);
		virtual ~FDMSupportGroup();

		void setVisibility(bool visibility);
		void createSupport(FDMSupportParam& param);
		void clearSupports();
		void removeSupport(FDMSupport* support);
		void setSupportState(FDMSupport* support, int state);
		int  fdmSupportNum();

		void save(QDataStream& stream);
		void load(QDataStream& stream);
		void buildFDMSup(QList<FDMSupport*> fdmSups);
		QList<FDMSupport*> FDMSupports();
		void displayFDMSup();
		void move2MainThread(QThread* mainThread);

		trimesh::TriMesh* createFDMSupportMesh();
		void createFDMSupportMeshes(std::vector<trimesh::TriMesh*>& meshes);
		FDMSupport* face2Support(int faceID, QVector3D* position = nullptr, QVector3D* normal = nullptr);
		int primitiveNum() override;
	protected:
		void onGlobalMatrixChanged(const QMatrix4x4& globalMatrix) override;
		void faceBaseChanged(int faceBase) override;
	protected:
		qtuser_3d::SupportChunkGroupEntity* m_chunkGroupEntity;

		QList<FDMSupport*> m_fdmSupports;
	};
}
#endif // CREATIVE_KERNEL_FDMSUPPORTGROUP_1595470832654_H