#ifndef CX3DPROJECTMANAGER_H
#define CX3DPROJECTMANAGER_H

#include <QUrl>
#include <QMap>
#include <QObject>
#include "trimesh2/TriMesh.h"
#include "quazip/quazip.h"
#include "quazip/quazipfile.h"
#include "qtusercore/module/progressor.h"
#include <QXmlStreamWriter>
#include "data/modeln.h"
#include <unordered_map>
#include <unordered_set>

namespace cx3d {
    struct MeshInfo {
        QString meshPath;
        int meshIndex = 0;
        std::string unidId;
    };

    struct MatrixInfo {
        QVector3D Position;
        QVector3D Scale;
        QQuaternion Rotate;
    };

    struct SupportInfo {
        QString supPath;
        QString supName;
    };

    struct SpreadInfo {
        QString sprPath;
        QString sprName;
    };

    struct AdaptLayerInfo {
        QString layerPath;
        QString layerName;
    };

    struct ObjectSetInfo {
        QString setPath;
        QString setName;
    };

    struct ModelInfo {
        QString modelName; //模型名字
        MeshInfo mesh; //网格信息
        MatrixInfo matrix; //矩阵信息
        SupportInfo support; //支撑信息
        SpreadInfo spread; //涂色信息
        AdaptLayerInfo layer; //可变层高
        ObjectSetInfo setting; //对象参数设置
        int colorIndex = 0; //耗材颜色索引
    };

    struct EngineInfo {
        QString profileUrl;//涵盖挤出机，工艺配置，机型，耗材
    };

    struct PlateInfo {
        std::map<std::string, std::string> plateParameter;
    };

    struct contentXML {
        QString version = "1.0.0"; //工程文件版本号
        QString slicerType = "FDM"; //工程切片类型（FDM/SLA）
        EngineInfo engineInfo; //工程文件只有一种引擎类型
        std::vector<ModelInfo> modelInfos; //多个模型信息
        std::vector<PlateInfo> plateInfos; //多个打印盘信息
        QString platesUrl; //多盘信息临时字段
    };

    struct SpreadData {
        std::vector<std::string> facetColors; //颜色
        std::vector<std::string> facetSeams; //缝隙
        std::vector<std::string> facetSupports; //支撑
    };

    struct ModelData {
        QString modelName; //模型名字
        trimesh::TriMesh mesh; //网格数据
        MatrixInfo matrix; //旋转矩阵
        SpreadData spread; //涂抹数据
        std::vector<double> adaptedHeights; //可变层高
        std::map<std::string, std::string> processSettings; //对象参数管理
    };

    enum class FieldName {
        PARAM, //包含挤出机 机型 耗材 工艺配置相关参数
        MODEL, //包含网格 支撑 涂色 自适应层高 对象参数管理等参数
        PLATE, //包含多盘 场景等相关参数
    };

    class Cx3dFileProject : public QObject {
        Q_OBJECT
    private:
        void saveModelMesh(const QString& strName, const QString& strPathName, creative_kernel::ModelN* modeln);
        void saveFieldData(const QByteArray& data, const QString& fieldFileName);
        void saveProjectXML(const QString& contentName, const QString& strPathName);

        void loadProjectXML();
        void loadModelNData(const QString& dirctoryPath);
        void loadFieldData();
        void loadGeometryMeshData(const QString& directoryPath);

        //void mesh2DocObject(trimesh::TriMesh* mesh);
        //QString convertMatrixToString(QMatrix4x4 matrix);

    public:
        enum EXPORTERROR { ZIP_ERROR, FILEIO_ERROR };
        explicit Cx3dFileProject(const QString& path, qtuser_core::Progressor* progressor, QObject* parent = nullptr, bool isBackup = false, bool isWrite = true);
        ~Cx3dFileProject();
        void setSlicerType(const QString& type = "FDM");
        void setFieldData(const FieldName& name, const QByteArray& data);
        void setModelData(const QList<creative_kernel::ModelN*>& models);
        QString getSlicerType()const;
        QString getEngineType()const;
        QString getEngineVersion()const;
        void getAllEngineTypes(std::vector<QString>& types);
        void getFieldData(const FieldName& name, QByteArray& data)const;
        void getModelData(QList<creative_kernel::ModelN*>& modelDatas)const;
        void saveCx3dProject(const QString& version = QStringLiteral("1.0.0"));
        void readCx3dProject();
    signals:
        void onError(EXPORTERROR error);
    public slots:

    private:
        QString m_projectPath;
        bool m_backup = false;
        bool m_mode = true;
        QuaZip* m_quaZip = nullptr;
        QuaZipFile* m_modelFile = nullptr;
        QXmlStreamWriter* m_xmlWriter = nullptr;
        QXmlStreamReader* m_xmlReader = nullptr;
        qtuser_core::Progressor* m_progress = nullptr;
        std::unordered_map<std::string, int> m_meshPtrs;
        std::unordered_map<int, QString> m_meshUrls;
        std::unordered_set<QString> m_engineSet = { "creality", "cura", "orca", "prusa", "bambu", "ideamaker", "superslicer", "ffslicer", "simplify", "craftware" };
    public:
        contentXML* m_contentXml = nullptr;
        std::map<FieldName, QByteArray> m_datas;
        QList<creative_kernel::ModelN*> m_models;

        QMap<int, TriMeshPtr> m_geometryMeshDatas;
        QList<cxkernel::ModelNDataPtr> m_modelnDatas;
        QList<std::vector<double>> m_layerDatas;
        QList<us::USettings*> m_objectSettings;
    };

}

#endif // CX3DPROJECTMANAGER_H
