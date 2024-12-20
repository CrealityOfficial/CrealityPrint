#ifndef CX3DREADER_H
#define CX3DREADER_H

#include <QObject>
#include <QUrl>
#include "quazip/quazip.h"
#include "quazip/quazipfile.h"
#include <QXmlStreamReader>
#include <QMap>

class Cx3dReader : public QObject
{
    Q_OBJECT
public:
    enum IMPORTERROR { ZIP_ERROR,FILEIO_ERROR} ;
    explicit Cx3dReader(QString path,QObject *parent = nullptr);
    ~Cx3dReader();
signals:
    void onError(IMPORTERROR error);
public slots:
private:
    QMatrix4x4 convertStringToMatrix(QString sMatrix);
private:
    QuaZip m_zip;
    QuaZipFile *m_pModelFile;
    QXmlStreamReader *m_pXmlReader;
};

#endif // CX3DWRITER_H
