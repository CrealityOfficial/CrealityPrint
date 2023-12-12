#ifndef _QML_ENTRIES_1589421179475_H
#define _QML_ENTRIES_1589421179475_H
#include "qtusercore/qtusercoreexport.h"
#include <QtCore/QAbstractListModel>
#include "qtusercore/plugin/qmlentry.h"

class QTUSER_CORE_API QmlEntries : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit QmlEntries(QObject* parent = nullptr);
	virtual ~QmlEntries();

	void add(QmlEntry* entry);
	void append(QmlEntry* entry);
	void remove(QmlEntry* entry);
	void clearButFirst();
	void removeFirstOne();

	Q_INVOKABLE QObject* rawData(int index);
protected:
	int rowCount(const QModelIndex& parent) const override;
	int columnCount(const QModelIndex& parent) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role) override;
	QHash<int, QByteArray> roleNames() const override;

protected:
	QList<QmlEntry*> m_entries;

	QHash<int, QByteArray> m_rolesName;
};

#endif // _QML_ENTRIES_1589421179475_H
