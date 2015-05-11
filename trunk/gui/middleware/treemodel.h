#pragma once

#include <QtCore/QAbstractItemModel>
#include <QtWidgets/QItemDelegate>

#include "nodeitem.h"

namespace vehicle {
namespace middleware {

class TreeItem
{
public:
    enum Roles
    {
        Kind = Qt::UserRole + 1
    };

    explicit TreeItem(AOTree* tree, AOTree::node_t* node, TreeItem* parent = 0);
    ~TreeItem();

    TreeItem* child(int number);
    TreeItem* parent();

    bool insertChildren(int position, int count = 1);
    bool removeChildren(int position, int count = 1);

    int childNumber() const;
    int childCount() const;

    QVariant data(int column) const;
    bool setData(int column, const QVariant& value);

    core::NodeKind kind() const;
    void setKind(core::NodeKind kind);

private:
    QVector<TreeItem*> children_;
    AOTree::node_t* node_;
    TreeItem* parent_;
    AOTree* tree_;
};

class TreeItemDelegate : public QItemDelegate
{
public:
    explicit TreeItemDelegate(QObject* parent = 0);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

class TreeModel : public QAbstractItemModel
{
public:
    explicit TreeModel(AOTree* tree, QObject* parent = 0);
    ~TreeModel();

    Qt::ItemFlags flags(const QModelIndex& index) const;

    QVariant data(const QModelIndex& index, int role) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& index) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    bool insertRows(int position, int rows, const QModelIndex& parent = QModelIndex());
    bool removeRows(int position, int rows, const QModelIndex& parent = QModelIndex());

private:
    TreeItem* item(const QModelIndex& index) const;

    TreeItem* root_;
};

} // namespace middleware
} // namespace vehicle
