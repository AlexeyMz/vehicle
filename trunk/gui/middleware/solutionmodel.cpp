#include <QtCore/QCryptographicHash>
#include <QtCore/QLocale>
#include <QtGui/QGuiApplication>
#include <QtGui/QFontMetrics>

#include "solutionmodel.h"

namespace vehicle {
namespace middleware {

Solution::Solution(solution_iterator::solution_tree_t solution)
{
    initialize(solution);
}

Solution::Solution(const Solution& solution)
{
    fullDescription_ = solution.fullDescription().toStringList();
    shortDescription_ = solution.shortDescription();
    price_ = solution.price();
    model_ = solution.model();
    mark_ = solution.mark();
    hash_ = solution.hash();
}

void Solution::initialize(solution_iterator::solution_tree_t solution)
{
    auto root = solution.getRoot();

    auto markNode = root->getValue();
    Q_ASSERT(markNode.hasChoice);
    mark_ = QString::fromStdString(markNode.node->child(markNode.index)->getValue().name());

    auto modelNode = root->child(markNode.index)->child(0)->getValue();
    Q_ASSERT(modelNode.hasChoice);
    model_ = QString::fromStdString(modelNode.node->child(modelNode.index)->getValue().name());

    price_ = root->subtreeKey();
    shortDescription_.clear();
    fullDescription_.clear();

    QByteArray hash;
    hash.append(mark_);
    hash.append(model_);

    static std::function<void(const solution_iterator::solution_node_t*, QStringList*, QString*, QByteArray*)> expandNode =
    [](const solution_iterator::solution_node_t* node, QStringList* model, QString* detailed, QByteArray* hash)
    {
        Q_ASSERT(node && model && hash);

        auto choice = node->getValue();
        auto childNode = node;
        if(choice.hasChoice)
        {
            childNode = node->child(choice.index);
            QString name = QString::fromStdString(choice.node->getValue().name());
            QString value = QString::fromStdString(childNode->getValue().node->getValue().name());
            int price = childNode->getValue().node->ownKey().getAsInteger();
            model->append("<b>" + name + "</b>: " + value + (price > 0 ? " (" + QLocale().toCurrencyString(price) + ")" : ""));
            hash->append(name + value);

            // skip mark and model in a short description
            if(model->size() > 2)
            {
                detailed->append(name + ": " + value + "     ");
                *detailed = QFontMetrics(QGuiApplication::font()).elidedText(*detailed, Qt::ElideRight, 500);
            }
        }

        for(auto child : *childNode)
            expandNode(child, model, detailed, hash);
    };

    for(auto child : *root)
        expandNode(child, &fullDescription_, &shortDescription_, &hash);

    hash_ = QCryptographicHash::hash(hash, QCryptographicHash::Sha1);
}

QByteArray Solution::hash() const
{
    return hash_;
}

SolutionModel* SolutionModel::create(solution_iterator solutions, QObject* parent)
{
    SolutionModel* model = new SolutionModel(parent);
    if(solutions.solutionCount())
    {
        do {
            Solution* solution = new Solution(solutions.currentSolution());
            model->addSolution(solution);
        } while(solutions.nextSolution());
    }
    return model;
}

SolutionModel::SolutionModel(QObject* parent) : QAbstractListModel(parent), sortOrder_(-1)
{
    roles_[ShortDescriptionRole] = "ShortDescription";
    roles_[FullDescriptionRole] = "FullDescription";
    roles_[PriceRole] = "Price";
    roles_[ModelRole] = "Model";
    roles_[MarkRole] = "Mark";
}

SolutionModel::~SolutionModel()
{
    clear();
}

void SolutionModel::recomputeToFit(SolutionModel* model)
{
    // Новая модель не должна быть отсортирована
    Q_ASSERT(model->sortOrder_ == -1);

    // Сортируем ее в соответствии с текущей моделью
    if(sortOrder_ != model->sortOrder_)
        model->sort(0, static_cast<Qt::SortOrder>(sortOrder_));

    auto it = solutions_.begin();
    int row = 0;
    while(it != solutions_.end())
    {
        if(!model->solutionsHash_.contains(solutions_[row]->hash()))
        {
            solutionsHash_.remove(solutions_[row]->hash());
            beginRemoveRows(QModelIndex(), row, row);
            Solution* tmp = *it;
            it = solutions_.erase(it);
            delete tmp;
            endRemoveRows();
        }
        else
        {
            ++row;
            it++;
        }
    }

    for(int i = 0; i < model->solutions_.size(); ++i)
        if(!solutionsHash_.contains(model->solutions_[i]->hash()))
            addSolution(new Solution(*model->solutions_[i]));
}

int SolutionModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return solutions_.count();
}

void SolutionModel::sort(int column, Qt::SortOrder order)
{
    if(column == 0 && sortOrder_ != order)
    {
        // Значит сортировка уже производилась и необходимо инвертировать массив
        if(sortOrder_ != -1)
            for(int i = 0; i < solutions_.size() / 2; ++i)
            {
                beginMoveRows(QModelIndex(), i, i, QModelIndex(), rowCount());
                solutions_.push_back(solutions_.takeFirst());
                endMoveRows();
            }
        else
        {
            // Пузырёк
            for(int i = 0; i < solutions_.size() - 1; ++i)
                for(int j = 0; j < solutions_.size() - i - 1; ++j)
                    if(order == Qt::AscendingOrder)
                    {
                        if(solutions_[j]->price() > solutions_[j + 1]->price())
                        {
                            beginMoveRows(QModelIndex(), j, j, QModelIndex(), j + 2);
                            Solution* tmp = solutions_[j];
                            solutions_.replace(j, solutions_[j + 1]);
                            solutions_.replace(j + 1, tmp);
                            endMoveRows();
                        }
                    }
                    else
                    {
                        if(solutions_[j]->price() < solutions_[j + 1]->price())
                        {
                            beginMoveRows(QModelIndex(), j, j, QModelIndex(), j + 2);
                            Solution* tmp = solutions_[j];
                            solutions_.replace(j, solutions_[j + 1]);
                            solutions_.replace(j + 1, tmp);
                            endMoveRows();
                        }
                    }
        }
        endResetModel();
        sortOrder_ = order;
    }
}

QVariant SolutionModel::data(const QModelIndex& index, int role) const
{
    if(index.row() < 0 || index.row() >= rowCount())
        return QVariant();

    switch(role)
    {
    case SolutionModel::ShortDescriptionRole :
        return solutions_[index.row()]->shortDescription();
    case SolutionModel::FullDescriptionRole :
        return solutions_[index.row()]->fullDescription();
    case SolutionModel::PriceRole :
        return solutions_[index.row()]->price().getAsInteger();
    case SolutionModel::ModelRole :
        return solutions_[index.row()]->model();
    case SolutionModel::MarkRole :
        return solutions_[index.row()]->mark();
    default :
        Q_UNREACHABLE();
        return QVariant();
    }
}

void SolutionModel::addSolution(Solution* solution)
{
    if(sortOrder_ != -1)
    {
        // Бинарный поиск места вставки элемента
        int start = 0, end = solutions_.count() - 1;
        while(end - start > 1)
        {
            int mid = (start + end) / 2;
            if(sortOrder_ == Qt::AscendingOrder)
            {
                if(solutions_[mid]->price() < solution->price())
                    start = mid;
                else
                    end = mid;
            }
            else
            {
                if(solutions_[mid]->price() > solution->price())
                    start = mid;
                else
                    end = mid;
            }
        }

        if(start > (solutions_.count() - 1)/ 2)
        {
            beginInsertRows(QModelIndex(), start, start);
            solutionsHash_[solution->hash()] = solution;
            solutions_.insert(start, solution);
        }
        else
        {
            beginInsertRows(QModelIndex(), start + 1, start + 1);
            solutionsHash_[solution->hash()] = solution;
            solutions_.insert(start + 1, solution);
        }
        endInsertRows();
    }
    else
    {
        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        solutionsHash_[solution->hash()] = solution;
        solutions_.push_back(solution);
        endInsertRows();
    }
}

void SolutionModel::clear()
{
    beginResetModel();
    qDeleteAll(solutions_);
    solutionsHash_.clear();
    solutions_.clear();
    sortOrder_ = -1;
    endResetModel();
}

QHash<int,QByteArray> SolutionModel::roleNames() const
{
    Q_ASSERT(!roles_.isEmpty());
    return roles_;
}

}
}
