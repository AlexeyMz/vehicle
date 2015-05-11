#include <QtGui/QLinearGradient>
#include <QtGui/QPainter>
#include <QtWidgets/QStyle>

#include "datamodel/AndOrTree.hpp"
#include "treemodel.h"

namespace vehicle {
using namespace core;
namespace middleware {

enum class Columns : int
{
    Title = 0,
    Price,

    Quantity
};

TreeItem::TreeItem(AOTree* tree, AOTree::node_t* node, TreeItem* parent) : parent_(parent), tree_(tree), node_(node)
{
    Q_ASSERT(tree && node);

    if(!parent)
        children_.push_back(new TreeItem(tree, node, this));
    else
        for(size_t i = 0; i < node->childCount(); ++i)
            children_.push_back(new TreeItem(tree, node->child(i), this));
}

TreeItem::~TreeItem()
{
    qDeleteAll(children_);
}

TreeItem* TreeItem::child(int number)
{
    return number >= 0 && number < children_.size() ? children_[number] : nullptr;
}

int TreeItem::childCount() const
{
    return children_.count();
}

int TreeItem::childNumber() const
{
    return parent_ ? parent_->children_.indexOf(const_cast<TreeItem * const>(this)) : 0;
}

bool TreeItem::insertChildren(int position, int count)
{
    if(position < 0 || position > children_.size())
        return false;

    if(kind() == NodeKind::NONE)
        setKind(NodeKind::AND);

    for(int row = 0; row < count; ++row)
    {
        AOTree::node_t* child = tree_->create(NodeKind::NONE, dec::decimal2(0), NodeItem(QObject::tr("Name").toStdString()));
        node_->attach(child);

        children_.insert(position, new TreeItem(tree_, child, this));
    }

    return true;
}

bool TreeItem::removeChildren(int position, int count)
{
    if(position < 0 || position + count > children_.size())
        return false;

    for(int row = 0; row < count; ++row)
    {
        AOTree::node_t* child = node_->child(position + row);
        for(size_t i = 0; i < child->childCount(); ++i)
            delete child->child(i);
        delete child;

        delete children_.takeAt(position);
    }

    if(childCount() == 1)
        setKind(NodeKind::AND);
    else if(childCount() == 0)
        setKind(NodeKind::NONE);

    return true;
}

QVariant TreeItem::data(int column) const
{
    switch(column)
    {
    case Columns::Title :
        return QString(node_->getValue().name().c_str());
    case Columns::Price :
        return QString::number(node_->ownKey().getAsInteger());
    case TreeItem::Kind :
        return static_cast<int>(kind());
    }

    return QVariant();
}

bool TreeItem::setData(int column, const QVariant& value)
{
    switch(column)
    {
    case Columns::Title :
        node_->getValue().setName(value.toString().toStdString());
        return true;
    case Columns::Price :
    {
        bool ok = false;
        int price = value.toInt(&ok);
        if(ok && price >= 0)
        {
            node_->setOwnKey(dec::decimal2(price));
            return true;
        }
        else
            return false;
    }
    case TreeItem::Kind :
        setKind(static_cast<NodeKind>(value.toInt()));
        return true;
    default :
        return false;
    }
}

NodeKind TreeItem::kind() const
{
    return node_->getKind();
}

void TreeItem::setKind(NodeKind kind)
{
    node_->setKind(kind);
}

TreeItem* TreeItem::parent()
{
    return parent_;
}

TreeItemDelegate::TreeItemDelegate(QObject* parent) : QItemDelegate(parent)
{
}

void TreeItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if(index.column() == static_cast<int>(Columns::Title) || index.column() == static_cast<int>(Columns::Price))
    {
        bool drawBorder = false;
        painter->save();

        if(option.state & QStyle::State_Selected)
        {
            //qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #ffcc00, stop: 1 #f5770a);
            QLinearGradient gradient(option.rect.topLeft(), option.rect.bottomLeft());
            gradient.setColorAt(0, QColor("#ffcc00"));
            gradient.setColorAt(1, QColor("#f5770a"));

            painter->fillRect(option.rect, QBrush(gradient));
            painter->setPen("#cc5500");
            drawBorder = true;
        }
        else if(option.state & QStyle::State_MouseOver)
        {
            //qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #ffeb99, stop: 1 #fac99d);
            QLinearGradient gradient(option.rect.topLeft(), option.rect.bottomLeft());
            gradient.setColorAt(0, QColor("#ffeb99"));
            gradient.setColorAt(1, QColor("#fac99d"));

            painter->fillRect(option.rect, QBrush(gradient));
            painter->setPen("#f7ad6d");
            drawBorder = true;
        }

        if(drawBorder)
        {
            painter->drawLine(option.rect.topLeft(), option.rect.topRight());
            painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());

            if(index.column() == static_cast<int>(Columns::Title))
                painter->drawLine(option.rect.bottomLeft(), option.rect.topLeft());
            else if(index.column() == static_cast<int>(Columns::Price))
                painter->drawLine(option.rect.bottomRight(), option.rect.topRight());
        }

        if(index.column() == static_cast<int>(Columns::Title))
        {
            QString prefix;
            switch(index.data(TreeItem::Kind).toInt())
            {
            case NodeKind::AND :
                painter->setPen("#9e0000");
                prefix = "[&] ";
                break;
            case NodeKind::OR :
                painter->setPen("#0c3696");
                prefix = "[||] ";
                break;
            case NodeKind::NONE :
                painter->setPen("#0bad00");
                break;
            default:
                break;
            }

            if(option.state & QStyle::State_Selected)
                painter->setPen(Qt::white);

            painter->drawText(option.rect, "  " + prefix + index.data(Qt::DisplayRole).toString());
        }
        else if(index.column() == static_cast<int>(Columns::Price))
        {
            if(option.state & QStyle::State_Selected)
                painter->setPen(Qt::white);
            else
                painter->setPen(Qt::black);

            painter->drawText(option.rect, "  " + index.data(Qt::DisplayRole).toString());
        }

        painter->restore();
    }
    else
        QItemDelegate::paint(painter, option, index);
}

TreeModel::TreeModel(AOTree* tree, QObject* parent) : QAbstractItemModel(parent)
{
    root_ = new TreeItem(tree, tree->getRoot(), nullptr);
}

TreeModel::~TreeModel()
{
    delete root_;
}

int TreeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(Columns::Quantity);
}

QVariant TreeModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid())
        return QVariant();
    else if(role != Qt::DisplayRole && role != Qt::EditRole)
    {
        switch(role)
        {
        case TreeItem::Kind:
            return item(index)->data(role);
        }

        return QVariant();
    }
    else
        return item(index)->data(index.column());
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;

    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

TreeItem* TreeModel::item(const QModelIndex &index) const
{
    if(index.isValid())
    {
        TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
        if(item)
            return item;
    }
    return root_;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if(parent.isValid() && parent.column() != 0)
        return QModelIndex();

    TreeItem* child = item(parent)->child(row);
    if(child)
        return createIndex(row, column, child);
    else
        return QModelIndex();
}

int TreeModel::rowCount(const QModelIndex& parent) const
{
    return item(parent)->childCount();
}

bool TreeModel::insertRows(int position, int rows, const QModelIndex& index)
{
    TreeItem* parent = item(index);
    bool success = false;

    // Добавлять строки узлу Mark нельзя
    if(parent == root_)
        return success;

    // Добавлять строки детям узла Mark тоже нельзя
    // (у них всегда должен быть только один дочерний узел - Model)
    for(int i = 0; i < root_->child(0)->childCount(); ++i)
        if(root_->child(0)->child(i) == parent)
            return success;

    beginInsertRows(index, position, position + rows - 1);
    success = parent->insertChildren(position, rows);
    endInsertRows();

    return success;
}

bool TreeModel::removeRows(int position, int rows, const QModelIndex& index)
{
    TreeItem* parent = item(index);
    bool success = false;

    // Удалять строки у корня дерева нельзя (т.е. узел Mark)
    if(parent == root_)
        return success;

    // Удалять строки у детей корня дерева тоже нельзя
    // (у них всегда должен быть только один дочерний узел - Model)
    for(int i = 0; i < root_->child(0)->childCount(); ++i)
        if(root_->child(0)->child(i) == parent)
            return success;

    beginRemoveRows(index, position, position + rows - 1);
    success = parent->removeChildren(position, rows);
    endRemoveRows();

    return success;
}

QModelIndex TreeModel::parent(const QModelIndex& index) const
{
    if(!index.isValid())
        return QModelIndex();

    TreeItem* child = item(index);
    TreeItem* parent = child->parent();

    if(parent == root_)
        return QModelIndex();
    else
        return createIndex(parent->childNumber(), 0, parent);
}

bool TreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if(role == Qt::EditRole)
    {
        if(item(index)->setData(index.column(), value))
        {
            emit dataChanged(index, index);
            return true;
        }
    }
    else
    {
        // Менять тип узла Mark нельзя
        if(item(index) == root_->child(0))
            return false;

        // Узла Model тоже
        for(int i = 0; i < root_->child(0)->childCount(); ++i)
            for(int j = 0; j < root_->child(0)->child(i)->childCount(); ++j)
                if(item(index) == root_->child(0)->child(i)->child(j))
                    return false;

        if(item(index)->setData(role, value))
        {
            emit dataChanged(index, index);
            return true;
        }
    }

    return false;
}

}
}
