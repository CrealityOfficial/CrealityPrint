#ifndef QTUSER_API_QUAZIPFILE_1660289457034_H
#define QTUSER_API_QUAZIPFILE_1660289457034_H
#include "qtusercore/qtusercoreexport.h"
#include <QtCore/QObject>
#include <QtCore/QIODevice>
#include <memory>
#include <functional>

typedef std::function<bool(QIODevice& device)> QuazipSubFunc;

class QuaZip;
namespace qtuser_core
{
	class QTUSER_CORE_API QuazipFile : public QObject
	{
		Q_OBJECT
	public:
		QuazipFile(QObject* parent = nullptr);
		virtual ~QuazipFile();

		void open(const QString& fileName);
		bool openSubFile(const QString& subFileName, QuazipSubFunc func);
	protected:
		std::unique_ptr<QuaZip> m_zip;
	};

	QTUSER_CORE_API bool unZipLocalFile(const QString& fileName, const QString& unZipFile);
	QTUSER_CORE_API bool zipLocalFile(const QString& fileName, const QString& zipFile);
	QTUSER_CORE_API bool compressDir(const QString& fileName, const QString& dir);
	QTUSER_CORE_API QStringList extractDir(const QString& fileName, const QString& dir);
	QTUSER_CORE_API QStringList getFileList(const QString& fileName);

	QTUSER_CORE_API QString CompressedFileFormat();
}

#endif // QCXUTIL_QUAZIPFILE_1660289457034_H
