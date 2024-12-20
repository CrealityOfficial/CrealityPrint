#include "materialcenter.h"
#include "internal/parameter/parameterpath.h"
#include "msbase/utils/flushvolume.h"
#include <QColor>
#include <internal/utils/flushvolume.cpp>
#include "interface/machineinterface.h"

namespace creative_kernel
{
    MaterialCenter::MaterialCenter(QObject* parent)
		: QObject(parent)
		, m_fultiplier(1.0f)
	{

	}

    MaterialCenter::~MaterialCenter()
	{

	}

	void MaterialCenter::initialize()
	{
		loadMaterialMeta(m_materialMetas);
		for (const MaterialData& meta : m_materialMetas)
		{
			if (!m_brands.contains(meta.brand))
				m_brands.append(meta.brand);
			if (!m_types.contains(meta.type))
				m_types.append(meta.type);
		}
		emit materialsChanged();
	}

	void MaterialCenter::reinitialize() {
		// TODO
	}

	QStringList MaterialCenter::types()
	{
		return m_types;
	}

	QStringList MaterialCenter::brands()
	{
		return m_brands;
	}

	QStringList MaterialCenter::materials()
	{
		QStringList matrials;
		Q_FOREACH(MaterialData material, m_materialMetas)
		{
			matrials.append(material.name);
		}
		return matrials;
	}

	const QList<MaterialData>& MaterialCenter::materialMetas()
	{
		return m_materialMetas;
	}

	QColor hexToRgb(const QString& hexColor) {
		// ȥ����ɫ�����е�ǰ׺ #
		QString color = hexColor;
		if (color.startsWith("#")) {
			color = color.mid(1);
		}

		// ������ɫ����ɫ����ɫ����
		QString redHex = color.mid(0, 2);
		QString greenHex = color.mid(2, 2);
		QString blueHex = color.mid(4, 2);

		// ��16���Ʒ���ת��Ϊʮ����
		bool ok;
		int red = redHex.toInt(&ok, 16);
		int green = greenHex.toInt(&ok, 16);
		int blue = blueHex.toInt(&ok, 16);

		// ��һ����0-255��Χ
		red = qBound(0, red, 255);
		green = qBound(0, green, 255);
		blue = qBound(0, blue, 255);

		// ����������RGB��ɫ
		return QColor(red, green, blue);
	}
	float MaterialCenter::getNozzleVolume()
	{
		return nozzle_volume();
	}

	void MaterialCenter::calc_flushing_volumes(std::vector<trimesh::vec3>& m_colours
		, /*out*/std::vector<float>& m_matrix
		, float flush_multiplier)
	{
		float  _nozzle_volume = nozzle_volume();
		//QSharedPointer<us::USettings> settings = createMachineSettings();
		//printer.nozzleCount = settings->value(QStringLiteral("machine_extruder_count"), "1").toInt();

		std::vector<bool> filament_is_support;
		msbase::calc_flushing_volumes(m_colours, filament_is_support, m_matrix, flush_multiplier, nozzle_volume());
	}
	QVariantList MaterialCenter::getFlushingVolumes(const QVariantList& hexColor, float flush_multiplier)
	{
		std::vector<trimesh::vec3> m_colours;
		QColor color;
		for (const QVariant& hexValue : hexColor) {
			color = hexToRgb(hexValue.toString());
			trimesh::vec3 color1;
			color1.x = static_cast<float>(color.red());
			color1.y = static_cast<float>(color.green());
			color1.z = static_cast<float>(color.blue());
			m_colours.push_back(color1);
		}
		std::vector<float> matrix;
		std::vector<bool> filament_is_support;
		msbase::calc_flushing_volumes(m_colours, filament_is_support, matrix, flush_multiplier, nozzle_volume());
		QVariantList matrixList;
		for (auto& value : matrix) {
			QString _value = QString::number(value,'f',0);
			matrixList.push_back(_value);
		}
		//setFlushingParams(matrixList);
		return matrixList;
	}

	QVariantList MaterialCenter::getFlushingVolumesDefault(int colorSize)
	{
		QVariantList matrixList;
		calc_flushing_volumes_default(colorSize, matrixList);
		//setFlushingParams(matrixList);
		return matrixList;
	}

	void MaterialCenter::calc_flushing_volumes_default(int colorSize, /*out*/QVariantList& m_matrix)
	{
		auto globalSetting = currentGlobalUserSettings();
		QString flush_volumes_matrix = "";
		std::vector<std::vector<QString>> _matrix;
		if (globalSetting){
			flush_volumes_matrix = globalSetting->value("flush_volumes_matrix","280");
			QString flush_volumes_multiplier = globalSetting->value("flush_multiplier", "1.0");
			float _flush_volumes_multiplier = flush_volumes_multiplier.toFloat();
			QStringList _flush_volumes_matrixs = flush_volumes_matrix.split(",");

			int _matrix_size = std::sqrt(_flush_volumes_matrixs.size());
			for (int i = 0; i < _matrix_size; i++){
				_matrix.push_back(std::vector<QString>());
				for (int j = 0; j < _matrix_size; j++)
				{
					float _value = _flush_volumes_matrixs.at(i * _matrix_size + j).toFloat();
					_value *= _flush_volumes_multiplier;
					_matrix.back().push_back(QString::number(_value, 'f', 0));
				}
			}
		}

		if (colorSize){
			for (int i = 0; i < colorSize; i++){
				for (int j = 0; j < colorSize; j++){
					if (i == j)
						m_matrix.push_back("0");
					else
						if (_matrix.size()>i && _matrix[i].size()>j)
							m_matrix.push_back(_matrix[i][j]);
						else
							m_matrix.push_back("280");
				}
			}
		}
	}

	void MaterialCenter::setFlushingParams(const QVariantList& matrixList)
	{
		auto globalSetting = currentGlobalUserSettings();
		QString hexColorString;
		QString matrixString;

		for (int i = 0; i < matrixList.size(); ++i) {
			const QVariant& matrix = matrixList.at(i);
			matrixString += matrix.toString();

			if (i != matrixList.size() - 1) {
				matrixString += ",";
			}
		}

		globalSetting->add("flush_volumes_matrix", matrixString, true);
	}

	void MaterialCenter::setFlushingParams(const QVariantList& matrixList, float flush_multiplier)
	{
		auto globalSetting = currentGlobalUserSettings();
		QString hexColorString;
		QString matrixString;

		for (int i = 0; i < matrixList.size(); ++i) {
			const QVariant& matrix = matrixList.at(i);
			float _value = matrix.toFloat();
			_value /= (flush_multiplier == 0 ? 1 : flush_multiplier);
			matrixString += QString::number(_value, 'f', 0);
			if (i != matrixList.size() - 1) {
				matrixString += ",";
			}
		}
		QString flush_volumes_matrix = globalSetting->value("flush_volumes_matrix","");
		if(flush_volumes_matrix != matrixString)
		{
			globalSetting->add("flush_volumes_matrix", matrixString, true);
		}
		
		globalSetting->add("flush_volumes_multiplier", QString::number(flush_multiplier, 'f', 2), true);
		globalSetting->add("flush_multiplier", QString::number(flush_multiplier, 'f', 2), true);
	}

	QVariantList MaterialCenter::editFlushingValue(float flush_multiplier)
	{
		auto globalSetting = currentGlobalUserSettings();
		QString flush_volumes_matrix = "";
		QVariantList  _matrixList;
		if (globalSetting) {
			flush_volumes_matrix = globalSetting->value("flush_volumes_matrix", "280");
			QStringList _flush_volumes_matrixs = flush_volumes_matrix.split(",");
			int _matrix_size = std::sqrt(_flush_volumes_matrixs.size());
			for (int i = 0; i < _matrix_size; i++) {
				for (int j = 0; j < _matrix_size; j++)
				{
					float _matrix = _flush_volumes_matrixs[i * _matrix_size + j].toFloat();
					if (m_fultiplier)
						_matrix /= m_fultiplier;
					_matrix *= flush_multiplier;
					QString _value = QString::number(_matrix, 'f', 0);
					_matrixList.push_back(_value);
				}
			}
			setFlushingParams(_matrixList);
		}

		m_fultiplier = flush_multiplier;

		emit dirtied();
		return _matrixList;
	}
	float MaterialCenter::getFlushingMultiplier()
	{
		return m_fultiplier;
	}

	void MaterialCenter::addExtruder(const QList<QColor>& hexColor)
	{
		std::vector<trimesh::vec3> m_colours;
		for (const auto& color : hexColor) {
			trimesh::vec3 color1;
			color1.x = static_cast<float>(color.red());
			color1.y = static_cast<float>(color.green());
			color1.z = static_cast<float>(color.blue());
			m_colours.push_back(color1);
		}
		auto globalSetting = currentGlobalUserSettings();
		QString flush_volumes_matrix = "";
		std::vector<std::vector<QString>> _matrix;
		if (globalSetting) {
			flush_volumes_matrix = globalSetting->value("flush_volumes_matrix", "280");
			QString flush_volumes_multiplier = globalSetting->value("flush_multiplier", "1.0");
			float _flush_volumes_multiplier = flush_volumes_multiplier.toFloat();
			QStringList _flush_volumes_matrixs = flush_volumes_matrix.split(",");

			int _matrix_size = std::sqrt(_flush_volumes_matrixs.size());
			++_matrix_size;
			for (int i = 0; i < _matrix_size; i++) {
				_matrix.push_back(std::vector<QString>());
				for (int j = 0; j < _matrix_size; j++)
				{
					float _value = 0.0f;
					if (i == _matrix_size - 1 || j == _matrix_size - 1)
					{
						_value = msbase::calc_flushing_volume(m_colours[i], m_colours[j], nozzle_volume());
					}
					else
					{
						_value = _flush_volumes_matrixs.at(i * (_matrix_size-1) + j).toFloat();
					}
					_value *= _flush_volumes_multiplier;
					_matrix.back().push_back(QString::number(_value, 'f', 0));
				}
			}

			QVariantList matrixList;
			for (int i = 0; i < _matrix_size; i++) {
				for (int j = 0; j < _matrix_size; j++) {
					if (i == j)
						matrixList.push_back("0");
					else
						if (_matrix.size() > i && _matrix[i].size() > j)
							matrixList.push_back(_matrix[i][j]);
						else
							matrixList.push_back("280");
				}
			}
			setFlushingParams(matrixList);
		}
	}
	void MaterialCenter::removeExtruder()
	{
		auto globalSetting = currentGlobalUserSettings();
		QString flush_volumes_matrix = "";
		std::vector<std::vector<QString>> _matrix;
		if (globalSetting) {
			flush_volumes_matrix = globalSetting->value("flush_volumes_matrix", "280");
			QString flush_volumes_multiplier = globalSetting->value("flush_volumes_multiplier", "1.0");
			float _flush_volumes_multiplier = flush_volumes_multiplier.toFloat();
			QStringList _flush_volumes_matrixs = flush_volumes_matrix.split(",");

			int _matrix_size = std::sqrt(_flush_volumes_matrixs.size());
			for (int i = 0; i < _matrix_size; i++) {
				_matrix.push_back(std::vector<QString>());
				for (int j = 0; j < _matrix_size; j++)
				{
					float _value = _flush_volumes_matrixs.at(i * _matrix_size + j).toFloat();
					_value *= _flush_volumes_multiplier;
					_matrix.back().push_back(QString::number(_value, 'f', 0));
				}
			}
			--_matrix_size;
			QVariantList matrixList;
			for (int i = 0; i < _matrix_size; i++) {
				for (int j = 0; j < _matrix_size; j++) {
					if (i == j)
						matrixList.push_back("0");
					else
						if (_matrix.size() > i && _matrix[i].size() > j)
							matrixList.push_back(_matrix[i][j]);
						else
							matrixList.push_back("280");
				}
			}
			setFlushingParams(matrixList);
		}
	}
	void MaterialCenter::setExtruderColor(const QList<QColor>& hexColor, int index)
	{
		std::vector<trimesh::vec3> m_colours;
		for (const auto& color : hexColor) {
			trimesh::vec3 color1;
			color1.x = static_cast<float>(color.red());
			color1.y = static_cast<float>(color.green());
			color1.z = static_cast<float>(color.blue());
			m_colours.push_back(color1);
		}
		auto globalSetting = currentGlobalUserSettings();
		QString flush_volumes_matrix = "";
		std::vector<std::vector<QString>> _matrix;
		if (globalSetting) {
			flush_volumes_matrix = globalSetting->value("flush_volumes_matrix", "280");
			QString flush_volumes_multiplier = globalSetting->value("flush_volumes_multiplier", "1.0");
			float _flush_volumes_multiplier = flush_volumes_multiplier.toFloat();
			QStringList _flush_volumes_matrixs = flush_volumes_matrix.split(",");

			int _matrix_size = std::sqrt(_flush_volumes_matrixs.size());
			for (int i = 0; i < _matrix_size; i++) {
				_matrix.push_back(std::vector<QString>());
				for (int j = 0; j < _matrix_size; j++)
				{
					float _value = 0.0f;
					if (i == index || j == index)
					{
						_value = msbase::calc_flushing_volume(m_colours[i], m_colours[j], nozzle_volume());
					}
					else
					{
						_value = _flush_volumes_matrixs.at(i * _matrix_size + j).toFloat();
					}
					_value *= _flush_volumes_multiplier;
					_matrix.back().push_back(QString::number(_value, 'f', 0));
				}
			}
			QVariantList matrixList;
			for (int i = 0; i < _matrix_size; i++) {
				for (int j = 0; j < _matrix_size; j++) {
					if (i == j)
						matrixList.push_back("0");
					else
						if (_matrix.size() > i && _matrix[i].size() > j)
							matrixList.push_back(_matrix[i][j]);
						else
							matrixList.push_back("280");
				}
			}
			setFlushingParams(matrixList);
		}
	}
	void calculate_flush_matrix_and_set(const QString& materialColorsString, const QString& flushMatrixString, const QString& flushVectorString, float flush_multiplier)
	{
		auto globalSetting = currentGlobalUserSettings();
		QString matrixString;

		if (0)
		{
			QStringList materialColors = materialColorsString.split(",");
			int size = materialColors.size();
			if (size >= 2)
			{

				std::vector<trimesh::vec3> m_colours;
				QColor color;
				for (const QString& hexValue : materialColors) {
					color = hexToRgb(hexValue);
					trimesh::vec3 color1;
					color1.x = static_cast<float>(color.red());
					color1.y = static_cast<float>(color.green());
					color1.z = static_cast<float>(color.blue());
					m_colours.push_back(color1);
				}
				std::vector<float> matrix;
				std::vector<bool> filament_is_support;
				msbase::calc_flushing_volumes(m_colours, filament_is_support, matrix, flush_multiplier, nozzle_volume());

				for (int i = 0; i < matrix.size(); ++i) {
					float _value = matrix.at(i);
					_value /= (flush_multiplier == 0 ? 1 : flush_multiplier);
					matrixString += QString::number(_value, 'f', 0);
					if (i != matrix.size() - 1) {
						matrixString += ",";
					}
				}
			}
		}
		else {
			matrixString = flushMatrixString;
		}

		globalSetting->add("flush_volumes_matrix", matrixString, true);
		globalSetting->add("flush_volumes_multiplier", QString::number(flush_multiplier, 'f', 2), true);
		globalSetting->add("flush_multiplier", QString::number(flush_multiplier, 'f', 2), true);
	}
}
