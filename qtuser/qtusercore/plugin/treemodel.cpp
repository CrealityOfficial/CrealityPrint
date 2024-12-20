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

/*
    treemodel.cpp

    Provides a simple tree model to show how to create and use hierarchical
    models.
*/

#include "treeitem.h"
#include "treemodel.h"

#include "customtype.h"
#include <QStringList>
namespace qtuser_qml
{
//! [0]
TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_roleNameMapping[TreeModelRoleName] = "title";
    m_roleNameMapping[TreeModelRoleDescription] = "summary";

    QList<QVariant> rootData;
    rootData << "Title" << "Summary";
    rootItem = new TreeItem(rootData);
    setupModelData(rootItem);
}
QModelIndex TreeModel::getRoot()
{
    return index(0, 0, QModelIndex());
}
void TreeModel::addGroup(int index,QObject* group)
{
    QList<QVariant> columnData;
    columnData << newCustomType(tr("Model Group"), index)<<"G"<<QVariant::fromValue<QObject*>(group);
    TreeItem*childItem = new TreeItem(columnData,rootItem);
    addItem(getRoot(),childItem);

}
void TreeModel::addItem(QModelIndex parent,TreeItem* item)
{
    TreeItem* parentItem = static_cast<TreeItem*>(parent.internalPointer());
    if(parentItem==nullptr)
    {
        parentItem = rootItem;
    }
    beginInsertRows(parent, parentItem->childCount(), parentItem->childCount());
    parentItem->appendChild(item);
    endInsertRows();
    emit addNewItem(parent);
}

QModelIndex TreeModel::getModelIndex(QObject *entity)
{
    QModelIndex selectedIndex = QModelIndex();
    QModelIndexList list = QAbstractItemModel::match(
                    getRoot(), NodeObjectRole, QVariant::fromValue(entity), 1,
                    Qt::MatchRecursive | Qt::MatchWrap | Qt::MatchExactly);
    if (list.count())
            selectedIndex = list.at(0);

     return selectedIndex;
}
void TreeModel::addModel(QObject* model, QObject* group)
{
    QModelIndex parent = getModelIndex(group);
    if(parent.isValid())
    {//The second level and lower level nodes are deleted
         TreeItem* parentItem = getItem(parent);
        QList<QVariant> columnData;
        columnData << newCustomType(model->objectName(), parentItem->childCount())<<"M"<< QVariant::fromValue(model);
        TreeItem*childItem = new TreeItem(columnData,parentItem);
        addItem(parent,childItem);
    }
    //2020-11-17 lisugui
    else
    {
        //For the first level node (root node)
       QList<QVariant> columnData;
       columnData << newCustomType(model->objectName(), rootItem->childCount())<<"M"<< QVariant::fromValue(model);
       TreeItem*childItem = new TreeItem(columnData,rootItem);
       addItem(parent,childItem);

    }

    int row = rowCount();
    emit rowChanged(row);
    //end
}

void TreeModel::delModel(QModelIndex index)
{
    QModelIndex parentIndex = parent(index);
    TreeItem* parentItem = getItem(parentIndex);
    TreeItem* childItem = getItem(index);
    if (nullptr == childItem)
    {
        return;
    }

    if(parentItem)
    {
        //The second level and lower level nodes are deleted
        beginRemoveRows(parentIndex, index.row(), index.row());
            parentItem->removeChild(childItem);
         endRemoveRows();

    }
    //2020-11-17 lisugui   add wj for delete mesh crash bug
    else if(childItem) 
    {

        //For the first level node (root node)
        beginRemoveRows(parentIndex, index.row(), index.row());
            rootItem->removeChild(childItem);
         endRemoveRows();
    }

    //end
}

void TreeModel::delModel(QObject* model)
{
    QModelIndex index = getModelIndex(model);
    delModel(index);
    beginResetModel();
    endResetModel();

    int row = rowCount();
    emit rowChanged(row);
}

void TreeModel::rename(QObject* model)
{
    QModelIndex index = getModelIndex(model);

    QModelIndex parentIndex = parent(index);
    TreeItem* parentItem = getItem(parentIndex);
    TreeItem* childItem = getItem(index);
    if (nullptr == childItem)
    {
        return;
    }

    if (parentItem)
    {

    }
    //2020-11-17 lisugui   add wj for delete mesh crash bug
    else if (childItem)
    {
        QVariant varObj = childItem->data(0);
        CustomType* modelData = varObj.value<CustomType*>();
        modelData->setText(model->objectName());
    }

    int row = rowCount();
    emit rowChanged(row);
}
//! [0]

//! [1]
TreeModel::~TreeModel()
{
    delete rootItem;
}
//! [1]

//! [2]
int TreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}
//! [2]

//! [3]
QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != TreeModelRoleName && role != TreeModelRoleDescription && role!=NodeObjectRole)
        return QVariant();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    return item->data(role - Qt::UserRole - 1);
}
//! [3]

//! [4]
Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return QAbstractItemModel::flags(index);
}
//! [4]

//! [5]
QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}
//! [5]

//! [6]
QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}
//! [6]
TreeItem* TreeModel::getItem(QModelIndex index)
{
    return  static_cast<TreeItem*>(index.internalPointer());
}
//! [7]
QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
    TreeItem *parentItem = childItem->parentItem();

    if (parentItem == rootItem)
        return QModelIndex();
    if (parentItem)
    {
        return createIndex(parentItem->row(), 0, parentItem);
    }
    else {
        return QModelIndex();
    }
}
//! [7]

//! [8]
int TreeModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());
   
    return parentItem->childCount();
}
//! [8]

QHash<int, QByteArray> TreeModel::roleNames() const
{
    return m_roleNameMapping;
}

QVariant TreeModel::newCustomType(const QString &text, int position)
{
    CustomType *t = new CustomType(this);
    t->setText(text);
    t->setIndentation(position);
    QVariant v;
    v.setValue(t);
    return v;
}
QObject* TreeModel::getItemObject(QModelIndex index)
{
    TreeItem *item = getItem(index);
    if(item)
    {
        return item->data(2).value<QObject*>();
    }
    return nullptr;

}
void TreeModel::setupModelData(TreeItem *parent)
{

/*
    QList<TreeItem*> parents;
    QList<int> indentations;
    parents << parent;
    creative_kernel::ModelSpace *pSpace = getKernel()->modelSpace();
    QList<creative_kernel::ModelGroup *> modelgroup = pSpace->modelGroups();
    QList<QVariant> columnData;
    int numb=0;
    //for(creative_kernel::ModelGroup *gp : modelgroup)
    {
        columnData << newCustomType("GroupX", numb);
        parents.last()->appendChild(new TreeItem(columnData, parents.last()));
        numb++;
    }

    int number = 0;

    while (number < lines.count()) {
        int position = 0;
        while (position < lines[number].length()) {
            if (lines[number].at(position) != ' ')
                break;
            position++;
        }

        QString lineData = lines[number].mid(position).trimmed();

        if (!lineData.isEmpty()) {
            // Read the column data from the rest of the line.
            QStringList columnStrings = lineData.split("\t", QString::SkipEmptyParts);
            QList<QVariant> columnData;
            for (int column = 0; column < columnStrings.count(); ++column)
                columnData << newCustomType(columnStrings[column], position);

            if (position > indentations.last()) {
                // The last child of the current parent is now the new parent
                // unless the current parent has no children.

                if (parents.last()->childCount() > 0) {
                    parents << parents.last()->child(parents.last()->childCount()-1);
                    indentations << position;
                }
            } else {
                while (position < indentations.last() && parents.count() > 0) {
                    parents.pop_back();
                    indentations.pop_back();
                }
            }

            // Append a new item to the current parent's list of children.
            parents.last()->appendChild(new TreeItem(columnData, parents.last()));
        }

        ++number;
    }
    */
}
}
