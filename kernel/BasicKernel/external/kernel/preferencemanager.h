#ifndef _PREFERENCE_MANAGER_H
#define _PREFERENCE_MANAGER_H
#include <QtCore/QObject>
#include <QVariant>
#include "basickernelexport.h"
namespace creative_kernel
{
	class BASIC_KERNEL_API PreferenceManager : public QObject
	{
		Q_OBJECT

		Q_PROPERTY(QString downloadModelPath READ downloadModelPath WRITE setDownloadModelPath NOTIFY valChanged)

	public:
		PreferenceManager(QObject* parent = nullptr);
		virtual ~PreferenceManager();


		QString downloadModelPath() const {
			return m_downloadModelPath;
		}
		void setDownloadModelPath(const QString path);

		Q_INVOKABLE void swithPathDialog();
		Q_INVOKABLE void acceptRepalceCache();

	Q_SIGNALS:
		void valChanged();
		void downPathChanged();
	private :
		QVariant getSettingValue(const QString& key, const QVariant& defaultValue = QVariant());
		void setSettingValue(const QString& key, const QVariant& value);
	private:
		//cloud
		QString m_downloadModelPath;											//创想云下载模型的默认路径
		QString m_lastModelPath;
		//bool m_exitLogin;														//退出软件时自动退出登录
	};

}

#endif // _PREFERENCE_MANAGER_H