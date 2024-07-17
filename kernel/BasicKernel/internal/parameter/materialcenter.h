#ifndef CREATIVE_KERNEL_PRINTMATERIALCENTER_1675841555856_H
#define CREATIVE_KERNEL_PRINTMATERIALCENTER_1675841555856_H
#include "data/header.h"

namespace creative_kernel
{
	class MaterialCenter : public QObject
	{
		Q_OBJECT
		Q_PROPERTY(QStringList materials READ materials NOTIFY materialsChanged)
	public:
		MaterialCenter(QObject* parent = nullptr);
		virtual ~MaterialCenter();

		void initialize();
		void reinitialize();
		Q_INVOKABLE QStringList types();
		Q_INVOKABLE QStringList brands();
		QStringList materials();

		const QList<MaterialData>& materialMetas();
		Q_INVOKABLE QVariantList getFlushingVolumes(const QVariantList& hexColor, float flush_multiplier = 1.0);
		Q_INVOKABLE QVariantList getFlushingVolumesDefault(int colorSize);
		Q_INVOKABLE void setFlushingParams(const QVariantList& matrixList);
		Q_INVOKABLE void setFlushingParams(const QVariantList& matrixList, float flush_multiplier);
		Q_INVOKABLE QVariantList editFlushingValue(float flush_multiplier = 1.0);
		Q_INVOKABLE float getFlushingMultiplier();
		Q_INVOKABLE float getNozzleVolume();
	protected:
		 void calc_flushing_volumes(
			 std::vector<trimesh::vec3>& m_colours  //ÿ����ɫ��RGB
			, /*out*/std::vector<float>& m_matrix   //����õ��ĳ�ˢ�� ���д洢    11 12 13 21 22 23 31 32 33...
			, float flush_multiplier = 1.0);        //����

		 void calc_flushing_volumes_default(int colorSize, /*out*/QVariantList& m_matrix);
	protected:
		QList<MaterialData> m_materialMetas;

		QStringList m_types;
		QStringList m_brands;

	private:
		float m_fultiplier = 1.0;

	signals:
		void materialsChanged();
		void dirtied();
	};

	void calculate_flush_matrix_and_set(const QString& materialColors, const QString& flush_matrix, const QString& flush_vector, float flush_multiplier);
}

#endif // CREATIVE_KERNEL_PRINTMATERIAL_1675841555856_H
