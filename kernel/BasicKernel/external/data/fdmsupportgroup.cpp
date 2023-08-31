#include "fdmsupportgroup.h"
#include "qtuser3d/entity/supportchunkgroupentity.h"
#include "qtuser3d/entity/supportchunkentity.h"

#include "data/fdmsupport.h"

#include "interface/spaceinterface.h"
#include "interface/reuseableinterface.h"

using namespace qtuser_3d;
namespace creative_kernel
{
	FDMSupportGroup::FDMSupportGroup(qtuser_3d::Node3D* parent)
		:Node3D(parent)
		, m_chunkGroupEntity(nullptr)
	{
		m_chunkGroupEntity = new SupportChunkGroupEntity();
		m_chunkGroupEntity->setProto(12, 100);

		m_chunkGroupEntity->setUsedEffect(creative_kernel::getCachedSupportEffect());
	}
	
	FDMSupportGroup::~FDMSupportGroup()
	{
		qDeleteAll(m_fdmSupports);
		if (m_chunkGroupEntity != nullptr)
		{
			delete m_chunkGroupEntity;
			m_chunkGroupEntity = nullptr;
		}
	}

	void FDMSupportGroup::setVisibility(bool visibility)
	{
		visibility ? spaceShow(m_chunkGroupEntity) : spaceHide(m_chunkGroupEntity);
	}

	void FDMSupportGroup::createSupport(FDMSupportParam& param)
	{
		SupportChunkEntity* chunkEntity = m_chunkGroupEntity->freeChunkEntity();
		if (chunkEntity)
		{
			int chunk = chunkEntity->freeChunk();
			assert(chunk >= 0);

			FDMSupport* support = new FDMSupport(this);
			support->setParam(param);
			support->setState(1);
			support->setChunk(chunkEntity, chunk);
			support->updateAll();

			m_fdmSupports.push_back(support);
		}
	}

	void FDMSupportGroup::clearSupports()
	{
		qDeleteAll(m_fdmSupports);
		m_fdmSupports.clear();

		m_chunkGroupEntity->clearAllChunk();
	}

	void FDMSupportGroup::removeSupport(FDMSupport* support)
	{
		m_fdmSupports.removeOne(support);
		delete support;
	}

	void FDMSupportGroup::setSupportState(FDMSupport* support, int state)
	{
		support->setState(state);
		support->updateFlag();
	}

	int FDMSupportGroup::fdmSupportNum()
	{
		return m_fdmSupports.size();
	}

	void FDMSupportGroup::save(QDataStream& stream)
	{
		if (!fdmSupportNum()) return;

		stream << fdmSupportNum();

		for (FDMSupport* fdmSup: m_fdmSupports)
		{
			FDMSupportParam& fdmParam = fdmSup->param();
			stream << fdmParam.bottom[0];
			stream << fdmParam.bottom[1];
			stream << fdmParam.bottom[2];

			stream << fdmParam.top[0];
			stream << fdmParam.top[1];
			stream << fdmParam.top[2];
			stream << fdmParam.radius;
		}
	}

	void FDMSupportGroup::load(QDataStream& stream)
	{
		int size = 0;
		stream >> size;
		if (size >0)
		{
			for (size_t i = 0; i < size; i++)
			{
				FDMSupportParam fdmParam;
				stream >> fdmParam.bottom[0];
				stream >> fdmParam.bottom[1];
				stream >> fdmParam.bottom[2];

				stream >> fdmParam.top[0];
				stream >> fdmParam.top[1];
				stream >> fdmParam.top[2];
				stream >> fdmParam.radius;
				FDMSupport* fdmSup = new FDMSupport();
				fdmSup->setParam(fdmParam);
				m_fdmSupports.push_back(fdmSup);
			}
		}
		displayFDMSup();
	}

	void FDMSupportGroup::buildFDMSup(QList<FDMSupport*>  fdmSups)
	{
		for (FDMSupport* oldfdmSup : fdmSups)
		{
			FDMSupportParam fdmParam= oldfdmSup->param();
			FDMSupport* fdmSup = new FDMSupport();
			fdmSup->setParam(fdmParam);
			m_fdmSupports.push_back(fdmSup);
		}
		displayFDMSup();
	}

	QList<FDMSupport*> FDMSupportGroup::FDMSupports()
	{
		return m_fdmSupports;
	}

	void FDMSupportGroup::displayFDMSup()
	{
		for (FDMSupport* support : m_fdmSupports)
		{
			SupportChunkEntity* chunkEntity = m_chunkGroupEntity->freeChunkEntity();
			if (chunkEntity)
			{
				int chunk = chunkEntity->freeChunk();
				assert(chunk >= 0);

				support->setState(1);
				support->setChunk(chunkEntity, chunk);
				support->updateAll();
			}
		}
	}

	void FDMSupportGroup::move2MainThread(QThread* mainThread)
	{
		if (m_chunkGroupEntity && mainThread)
		{
			m_chunkGroupEntity->moveToThread(mainThread);
			this->moveToThread(mainThread);
		}
	}

	trimesh::TriMesh* FDMSupportGroup::createFDMSupportMesh()
	{
		std::vector<ChunkBufferUser*> users;
		for (ChunkBufferUser* user : m_fdmSupports)
			users.push_back(user);

		if (users.size() == 0)
			return nullptr;

		size_t size = users.size();

		trimesh::TriMesh* supportMesh = new trimesh::TriMesh();

		int totalSize = 0;
		std::vector<trimesh::ivec2> startAndSizes;
		if (size > 0)
		{
			startAndSizes.resize(size);

			int start = 0;
			for (size_t i = 0; i < size; ++i)
			{
				ChunkBufferUser* user = users.at(i);
				trimesh::ivec2 c(start, 0);
				c.y = user->validSize();
				startAndSizes.at(i) = c;
				start += c.y;
			}

			totalSize = startAndSizes.back().x + startAndSizes.back().y;
		}

		supportMesh->vertices.resize(totalSize);
		for (size_t i = 0; i < size; ++i)
		{
			ChunkBufferUser* user = users.at(i);
			trimesh::ivec2 c = startAndSizes.at(i);

			user->fillBuffer((float*)(&supportMesh->vertices.at(c.x)));
		}

		//for (size_t i = 0; i < totalSize / 3; ++i)
		//	supportMesh->faces.push_back(trimesh::TriMesh::Face(3 * i, 3 * i + 1, 3 * i + 2));
		supportMesh->faces.reserve(totalSize / 3);
		for (size_t i = 0; i < totalSize / 3; ++i)
		{
			trimesh::TriMesh::Face f;
			f.x = (int)(3 * i);
			f.y = (int)(3 * i + 1);
			f.z = (int)(3 * i + 2);
			supportMesh->faces.push_back(f);
		}
		return supportMesh;
	}

	void FDMSupportGroup::createFDMSupportMeshes(std::vector<trimesh::TriMesh*>& meshes)
	{
		meshes.clear();

		std::vector<ChunkBufferUser*> users;
		//m_topo->traitChunkBufferUser(users);
		for (ChunkBufferUser* user : m_fdmSupports)
			users.push_back(user);

		if (users.size() == 0)
			return;

		size_t size = users.size();
		meshes.resize(size);

		for (size_t i = 0; i < size; ++i)
		{
			ChunkBufferUser* user = users.at(i);

			trimesh::TriMesh* mesh = new trimesh::TriMesh();
			mesh->vertices.resize(user->validSize());

			user->fillBuffer((float*)(&mesh->vertices.at(0)));

			mesh->need_bbox();
			meshes.at(i) = mesh;
		}
	}

	FDMSupport* FDMSupportGroup::face2Support(int faceID, QVector3D* position, QVector3D* normal)
	{
		for (FDMSupport* support : m_fdmSupports)
		{
			if (support->check(faceID))
			{
				if (position || normal)
				{
				}
				return support;
			}
		}
		return nullptr;
	}

	void FDMSupportGroup::onGlobalMatrixChanged(const QMatrix4x4& globalMatrix)
	{
		m_chunkGroupEntity->setPose(globalMatrix);
	}

	void FDMSupportGroup::faceBaseChanged(int faceBase)
	{
		QPoint _faceBase(faceBase, 0);
		m_chunkGroupEntity->setFaceBase(_faceBase);
	}

	int FDMSupportGroup::primitiveNum()
	{
		return 1000000;
	}
}