#include "quazipfile.h"
#include "quazip/quazip.h"
#include "quazip/quazipfile.h"
#include "quazip/JlCompress.h"

#include <QtCore/QDebug>

namespace qtuser_core
{
	QuazipFile::QuazipFile(QObject* parent)
		: QObject(parent)
	{

	}

	QuazipFile::~QuazipFile()
	{
		if (m_zip)
			m_zip->close();
		m_zip.reset();
	}

	void QuazipFile::open(const QString& fileName)
	{
		m_zip.reset(new QuaZip());
		m_zip->setZipName(fileName);
		if (!m_zip->open(QuaZip::mdUnzip))
			m_zip.reset();
	}

	bool QuazipFile::openSubFile(const QString& subFileName, QuazipSubFunc func)
	{
		if (!m_zip)
			return false;

		QuaZipFile file;

		m_zip->setCurrentFile(subFileName);
		file.setZip(m_zip.get());
		if (file.open(QIODevice::ReadOnly) && func)
		{
			bool result = func(file);
			file.close();
			return result;
		}

		file.close();
		return false;
	}

	bool unZipLocalFile(const QString& fileName, const QString& unZipFile)
	{
		QByteArray array = fileName.toLocal8Bit();
		const char* str = array.data();
		gzFile gzfile = gzopen(str, "rb");
		if (gzfile == NULL)
		{
			return false;
		}
		int len = 0;
		char buf[1024];
		QByteArray fileArray;
		while (len = gzread(gzfile, buf, 1024))
		{
			fileArray.append((const char*)buf, len);
		}
		gzclose(gzfile);

		QFile filePath(unZipFile);
		QByteArray gzipBateArray = "";
		if (filePath.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			filePath.write(fileArray);
		}

		return true;
	}

	bool zipLocalFile(const QString& fileName, const QString& zipFile)
	{
		QFile file(fileName);
		QByteArray gzipBateArray = "";
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			int len = 0;
			char buf[1024];
			while (len = file.read(buf, 1024))
			{
				gzipBateArray.append((const char*)buf, len);
			}
		}
		file.close();

		gzFile gzfp = gzopen(zipFile.toLocal8Bit(), "wb");
		if (!gzfp)
		{
			qDebug() << "gzopen error";
		}
		if (gzwrite(gzfp, gzipBateArray, gzipBateArray.size()) > 0)
		{
			qDebug() << "gzwrite ok";
		}
		gzclose(gzfp);

		return true;
	}

	bool compressDir(const QString& fileName, const QString& dir)
	{
		return JlCompress::compressDir(fileName, dir);
	}

	QStringList extractDir(const QString& fileName, const QString& dir)
	{
		return JlCompress::extractDir(fileName, dir);
	}

	QStringList getFileList(const QString& fileName)
	{
		return JlCompress::getFileList(fileName);
	}

	QString CompressedFileFormat() { return QStringLiteral("gz"); }
}
