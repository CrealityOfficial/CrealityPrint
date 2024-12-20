/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include "qtusercore/qtusercoreexport.h"
class TreeItem;
namespace qtuser_qml
{
//! [0]
class QTUSER_CORE_API TreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum TreeModelRoles
    {
        TreeModelRoleName = Qt::UserRole + 1,
        TreeModelRoleDescription,
        NodeObjectRole
    };

    explicit TreeModel( QObject *parent = 0);
    ~TreeModel();

    /* QAbstractItemModel interface */
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QHash<int, QByteArray> roleNames() const override;

    void addGroup(int index,QObject* group);
    void addModel(QObject* model, QObject* group = nullptr);
    Q_INVOKABLE void delModel(QModelIndex index);
    Q_INVOKABLE void delModel(QObject* model);

    Q_INVOKABLE void rename(QObject* model);

     QModelIndex getModelIndex(QObject *entity);
     QModelIndex getRoot();
     QObject* getItemObject(QModelIndex index);
private:
    QVariant newCustomType(const QString &text, int position);
    void setupModelData(TreeItem *parent);
    void addItem(QModelIndex parent,TreeItem* item);
    TreeItem* getItem(QModelIndex index);
    TreeItem *rootItem;
    QHash<int, QByteArray> m_roleNameMapping;
 signals:
    //void onDelModelTreeItem(QObject *item);
    void addNewItem(QModelIndex index);
    void rowChanged(int row);//by TCJ
};
//! [0]
}
#endif // TREEMODEL_H
