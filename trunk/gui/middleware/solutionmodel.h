#pragma once

#include <QtCore/QAbstractListModel>
#include "nodeitem.h"

namespace vehicle {
namespace middleware {

class Solution
{
public:
    explicit Solution(solution_iterator::solution_tree_t solution);
    Solution(const Solution& solution);

    void initialize(solution_iterator::solution_tree_t solution);
    QByteArray hash() const;

    ///
    /// \return QStringList
    ///
    inline QVariant fullDescription() const { return QVariant::fromValue(fullDescription_); }
    inline QString shortDescription() const { return shortDescription_; }
    inline AOTree::key_t price() const { return price_; }
    inline QString model() const { return model_; }
    inline QString mark() const { return mark_; }

private:
    QStringList fullDescription_;
    QString shortDescription_;
    AOTree::key_t price_;
    QByteArray hash_;
    QString model_;
    QString mark_;
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

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    void addSolution(Solution* solution);
    void clear();

protected:
    QHash<int,QByteArray> roleNames() const;

private:
    QHash<QString,Solution*> solutionsHash_;
    QVector<Solution*> solutions_;
    QHash<int,QByteArray> roles_;
    int sortOrder_;
};

} // namespace middleware
} // namespace vehicle
