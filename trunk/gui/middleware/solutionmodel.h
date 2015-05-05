#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QUrl>

#include "nodeitem.h"

namespace vehicle {
namespace middleware {

namespace internal {
struct SolutionInitializer
{
    QStringList fullDescription;
    QString shortDescription;
    AOTree::key_t price;
    QByteArray hash;
    QString model;
    QString mark;
};
}

class Solution
{
public:
    explicit Solution(solution_iterator::solution_tree_t solution);
    Solution(internal::SolutionInitializer&& solution);
    Solution(const Solution& solution);

    void initialize(solution_iterator::solution_tree_t solution);

    ///
    /// \return QStringList
    ///
    inline QVariant fullDescription() const { return QVariant::fromValue(data_.fullDescription); }
    inline QString shortDescription() const { return data_.shortDescription; }
    inline AOTree::key_t price() const { return data_.price; }
    inline QString model() const { return data_.model; }
    inline QString mark() const { return data_.mark; }
    inline QByteArray hash() const { return data_.hash; }

private:
    internal::SolutionInitializer data_;
};

class SolutionModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum SolutionRoles {
        FullDescriptionRole = Qt::UserRole + 1,
        ShortDescriptionRole,
        PriceRole,
        ModelRole,
        MarkRole,
        HashRole
    };

    static SolutionModel* create(solution_iterator solutions, QObject* parent = 0);

    explicit SolutionModel(QObject* parent = 0);
    ~SolutionModel();

    void recomputeToFit(SolutionModel* model);

    Q_INVOKABLE int rowCount(const QModelIndex& parent = QModelIndex()) const;
    Q_INVOKABLE void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
    Q_INVOKABLE QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    Q_INVOKABLE QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

    void addSolution(Solution* solution);
    void clear();

    Q_INVOKABLE bool save(const QUrl& file);
    Q_INVOKABLE bool load(const QUrl& file);
    Q_INVOKABLE QString lastError() const;
    Q_INVOKABLE void restore();

protected:
    QHash<int,QByteArray> roleNames() const;

private:
    SolutionModel* tempModel_;
    QString lastError_;
    bool tempMode_;

    QHash<QString,Solution*> solutionsHash_;
    QVector<Solution*> solutions_;
    QHash<int,QByteArray> roles_;
    int sortOrder_;
};

} // namespace middleware
} // namespace vehicle
