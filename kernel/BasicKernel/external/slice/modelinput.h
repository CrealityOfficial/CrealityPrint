#ifndef _US_MODELINPUT_1589340410146_H
#define _US_MODELINPUT_1589340410146_H
#include "us/usettings.h"
#include "data/header.h"

namespace creative_kernel
{
	class ModelInput : public QObject
	{
	public:
		ModelInput(QObject* parent = nullptr);
		virtual ~ModelInput();

		int id() const;
		void setID(int id);

		const QString& name() const;
		void setName(const QString& name);

		virtual int vertexNum() const;
		virtual float* position() const;
		virtual float* normal() const;
		virtual TriMeshPtr ptr();
		void setPtr(TriMeshPtr mesh);
		TriMeshPtr mesh();
		std::vector<trimesh::vec3> outline_ObjectExclude;
		virtual int triangleNum() const;
		virtual int* indices() const;
	protected:
		int m_id;
		QString m_name;
		TriMeshPtr m_mesh;

	public:
		us::USettings* settings;
	};

	class ModelGroupInput : public QObject
	{
	public:
		ModelGroupInput(QObject* parent = nullptr);
		virtual ~ModelGroupInput();

		void addModelInput(ModelInput* modelInput);

		QList<ModelInput*> modelInputs;
		us::USettings* settings;
	};
}

#endif // _US_MODELINPUT_1589340410146_H
