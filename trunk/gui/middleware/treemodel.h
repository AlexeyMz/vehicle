#pragma once

#include <QtCore/QAbstractItemModel>
#include <QtWidgets/QItemDelegate>

#include "nodeitem.h"

namespace vehicle {
namespace middleware {

///
/// \class TreeItem
/// \brief Класс, представляющий элемент редактируемого дерева
///
class TreeItem
{
public:
    enum Roles
    {
        Kind = Qt::UserRole + 1
    };

    explicit TreeItem(AOTree* tree, AOTree::node_t* node, TreeItem* parent = 0);
    ~TreeItem();

    ///
    /// \brief Возвращает дочерний элемент по индексу \p number
    /// \param number - номер дочернего элемента
    ///
    TreeItem* child(int number);
    ///
    /// \brief Возвращает родительский элемент
    ///
    TreeItem* parent();

    ///
    /// \brief Добавляет детей
    /// \param position - позиция для вставки
    /// \param count - количество детей (по умолчанию 1)
    /// \return false, в случае некорректной позиции \p position
    ///
    bool insertChildren(int position, int count = 1);
    ///
    /// \brief Удаляет детей
    /// \param position - позиция, с которой начинается удаление
    /// \param count - количество удаляемых детей (по умолчанию 1)
    /// \return false, в случае некорректной позиции \p position
    ///
    bool removeChildren(int position, int count = 1);

    ///
    /// \brief Возвращает позицию данного элемента
    /// в списке детей родителя или 0, если родителя нет
    ///
    int childNumber() const;
    ///
    /// \brief Возвращает список детей
    ///
    int childCount() const;

    ///
    /// \brief Возвращает данные элемента для заданного столбца \p column
    /// \param column - столбец
    ///
    QVariant data(int column) const;
    ///
    /// \brief Устанавливает данные элемента для заданного столбца \p column
    /// \param column - стольбец
    /// \param value - значение
    ///
    bool setData(int column, const QVariant& value);

    ///
    /// \brief Возвращает тип элемента \e NodeKind
    ///
    core::NodeKind kind() const;
    ///
    /// \brief Устанавливает тип элемента
    ///
    void setKind(core::NodeKind kind);

private:
    QVector<TreeItem*> children_;
    AOTree::node_t* node_;
    TreeItem* parent_;
    AOTree* tree_;
};

///
/// \class TreeItemDelegate
/// \brief Переопределенние стандартного делегата отрисовки
/// для отображения элементов дерева
///
class TreeItemDelegate : public QItemDelegate
{
public:
    explicit TreeItemDelegate(QObject* parent = 0);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

///
/// \class TreeModel
/// \brief Реализация стандартной модели данных
/// с определением всех чисто виртуальных функций.
/// Класс используется совместно со стандартным
/// представлением QTreeView.
///
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
