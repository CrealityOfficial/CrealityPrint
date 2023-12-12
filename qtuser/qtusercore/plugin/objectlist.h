#ifndef QTUSER_QML_OBJECTLIST_1609220096298_H
#define QTUSER_QML_OBJECTLIST_1609220096298_H
#include "qtusercore/qtusercoreexport.h"
#include <QtCore/QAbstractListModel>

namespace qtuser_qml
{
	class QTUSER_CORE_API ObjectList : public QAbstractListModel
	{
		Q_OBJECT
	public:
		explicit ObjectList(QObject* parent = nullptr);
		virtual ~ObjectList();

		void setItems(QList<QObject*> items);
	protected:
		int rowCount(const QModelIndex& parent) const override;
		int columnCount(const QModelIndex& parent) const override;
		QVariant data(const QModelIndex& index, int role) const override;
		bool setData(const QModelIndex& index, const QVariant& value, int role) override;
		QHash<int, QByteArray> roleNames() const override;

	protected:
		QList<QObject*> m_items;
		QHash<int, QByteArray> m_rolesName;
	};

}

#endif // QTUSER_QML_OBJECTLIST_1609220096298_H