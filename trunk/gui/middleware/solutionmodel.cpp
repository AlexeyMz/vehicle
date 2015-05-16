#include <QtCore/QCryptographicHash>
#include <QtCore/QLocale>

#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrinter>

#include <QtConcurrent/QtConcurrentRun>
#include <QtWidgets/QMessageBox>
#include <QtGui/QGuiApplication>
#include <QtGui/QTextDocument>
#include <QtGui/QFontMetrics>

#include "../utils/xmlparser.h"
#include "solutionmodel.h"

namespace vehicle {
namespace middleware {

Solution::Solution(solution_iterator::solution_tree_t solution)
{
    initialize(solution);
}

Solution::Solution(internal::SolutionInitializer&& solution)
{
    data_.fullDescription = qMove(solution.fullDescription);
    data_.shortDescription = qMove(solution.shortDescription);
    data_.price = qMove(solution.price);
    data_.model = qMove(solution.model);
    data_.mark = qMove(solution.mark);
    data_.hash = qMove(solution.hash);
}

Solution::Solution(const Solution& solution)
{
    data_.fullDescription = solution.fullDescription().toStringList();
    data_.shortDescription = solution.shortDescription();
    data_.price = solution.price();
    data_.model = solution.model();
    data_.mark = solution.mark();
    data_.hash = solution.hash();
}

void Solution::initialize(solution_iterator::solution_tree_t solution)
{
    auto root = solution.getRoot();

    auto markNode = root->getValue();
    Q_ASSERT(markNode.hasChoice);
    data_.mark = QString::fromStdString(markNode.node->child(markNode.index)->getValue().name());

    auto modelNode = root->child(markNode.index)->child(0)->getValue();
    Q_ASSERT(modelNode.hasChoice);
    data_.model = QString::fromStdString(modelNode.node->child(modelNode.index)->getValue().name());

    data_.price = root->subtreeKey();
    data_.shortDescription.clear();
    data_.fullDescription.clear();

    QByteArray hash;
    hash.append(data_.mark);
    hash.append(data_.model);

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
            if(model->size() > 1)
            {
                detailed->append(name + ": " + value + "     ");
                *detailed = QFontMetrics(QGuiApplication::font()).elidedText(*detailed, Qt::ElideRight, 500);
            }

            expandNode(childNode, model, detailed, hash);
        }
        else
            for(auto child : *childNode)
                expandNode(child, model, detailed, hash);
    };

    expandNode(root->child(root->getValue().index), &data_.fullDescription, &data_.shortDescription, &hash);
    data_.hash = QCryptographicHash::hash(hash, QCryptographicHash::Sha1).toHex();
}

SolutionModel* SolutionModel::create(solution_iterator solutions, QObject* parent)
{
    SolutionModel* model = new SolutionModel(parent);
    if(solutions.solutionCount())
    {
        do {
            Solution* solution = new Solution(solutions.currentSolution());
			//Q_ASSERT(!model->solutionsHash_.contains(solution->hash()));
			model->solutionsHash_[solution->hash()] = solution;
			model->solutions_.push_back(solution);
        } while(solutions.nextSolution());
    }
    return model;
}

SolutionModel::SolutionModel(QObject* parent) : QAbstractListModel(parent), sortOrder_(-1), tempModel_(nullptr), tempMode_(false), outdated_(false)
{
	connect(&sorting_, SIGNAL(started()), SIGNAL(sortingStarted()));
	connect(&sorting_, SIGNAL(finished()), SLOT(endSorting()));
	connect(&sorting_, SIGNAL(finished()), SIGNAL(sortingFinished()));

    roles_[ShortDescriptionRole] = "ShortDescription";
    roles_[FullDescriptionRole] = "FullDescription";
    roles_[PriceRole] = "Price";
    roles_[ModelRole] = "Model";
    roles_[MarkRole] = "Mark";
    roles_[HashRole] = "Hash";
}

SolutionModel::~SolutionModel()
{
	sorting_.waitForFinished();
    clear();
}

void SolutionModel::recomputeToFit(SolutionModel* model)
{
    Q_ASSERT(!tempMode_);
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
    return tempMode_ ? tempModel_->rowCount(parent) : solutions_.count();
}

void SolutionModel::sort(int column, Qt::SortOrder order)
{
	sorting_.waitForFinished();

    if(tempMode_)
    {
        tempModel_->sort(column, order);
        return;
    }

	if(column == 0 && sortOrder_ != order)
		sorting_.setFuture(QtConcurrent::run(this, &SolutionModel::startSorting, column, order));
}

void SolutionModel::startSorting(int column, Qt::SortOrder order)
{
	// Значит сортировка уже производилась и необходимо инвертировать массив
    if(sortOrder_ != -1)
		std::reverse(solutions_.begin(), solutions_.end());
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
                    }
                }
                else
                {
                    if(solutions_[j]->price() < solutions_[j + 1]->price())
                    {
                        Solution* tmp = solutions_[j];
                        solutions_.replace(j, solutions_[j + 1]);
                        solutions_.replace(j + 1, tmp);
                    }
                }
	}
	
	sortOrder_ = order;
}

void SolutionModel::endSorting()
{
	beginResetModel();
	endResetModel();
}

QVariant SolutionModel::data(const QModelIndex& index, int role) const
{
    if(tempMode_)
        return tempModel_->data(index, role);

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
    case SolutionModel::HashRole :
        return solutions_[index.row()]->hash();
    default :
        Q_UNREACHABLE();
        return QVariant();
    }
}

void SolutionModel::addSolution(Solution* solution)
{
    Q_ASSERT(!tempMode_);
    //Q_ASSERT(!solutionsHash_.contains(solution->hash()));

    if(sortOrder_ != -1)
    {
        quint32 first = 0, last = solutions_.count();
        if(last == 0)
        {
            beginInsertRows(QModelIndex(), first, first);
            solutionsHash_[solution->hash()] = solution;
            solutions_.push_back(solution);
            endInsertRows();
            return;
        }
        else
        {
            if(sortOrder_ == Qt::AscendingOrder)
            {
                if(solutions_.first()->price() > solution->price())
                {
                    beginInsertRows(QModelIndex(), first, first);
                    solutionsHash_[solution->hash()] = solution;
                    solutions_.push_front(solution);
                    endInsertRows();
                    return;
                }
                else if(solutions_.last()->price() < solution->price())
                {
                    beginInsertRows(QModelIndex(), last, last);
                    solutionsHash_[solution->hash()] = solution;
                    solutions_.push_back(solution);
                    endInsertRows();
                    return;
                }
            }
            else
            {
                if(solutions_.first()->price() < solution->price())
                {
                    beginInsertRows(QModelIndex(), first, first);
                    solutionsHash_[solution->hash()] = solution;
                    solutions_.push_front(solution);
                    endInsertRows();
                    return;
                }
                else if(solutions_.last()->price() > solution->price())
                {
                    beginInsertRows(QModelIndex(), last, last);
                    solutionsHash_[solution->hash()] = solution;
                    solutions_.push_back(solution);
                    endInsertRows();
                    return;
                }
            }
        }

        // Бинарный поиск места вставки элемента
        while(first < last)
        {
            quint32 mid = first + (last - first) / 2;

            if(sortOrder_ == Qt::AscendingOrder)
            {
                if(solution->price() <= solutions_[mid]->price())
                    last = mid;
                else
                    first = mid + 1;
            }
            else
            {
                if(solution->price() >= solutions_[mid]->price())
                    last = mid;
                else
                    first = mid + 1;
            }
        }

        beginInsertRows(QModelIndex(), last, last);
        solutionsHash_[solution->hash()] = solution;
        solutions_.insert(last, solution);
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

QModelIndex SolutionModel::index(int row, int column, const QModelIndex&) const
{
    if(tempMode_)
        return tempModel_->index(row, column);

    if(row < 0 || row >= rowCount())
        return QModelIndex();
    else
        return createIndex(row, column);
}

bool SolutionModel::save(const QUrl& file)
{
    bool ok = utils::XmlParser::instance()->saveSolutions(tempMode_ ? tempModel_ : this, file.toLocalFile());
    lastError_ = utils::XmlParser::instance()->lastError();
    return ok;
}

bool SolutionModel::saveSolution(int index, const QUrl& file)
{
    if(index < 0 || index >= solutions_.size())
    {
        lastError_ = tr("Cannot save configuration: incorrect index %1").arg(index);
        return false;
    }

    static QString htmlTemplate =
    QString("<body bgcolor=\"#ffda99\">"
            "<p align=\"center\"><font color=\"#cf6311\"><h1>%1</h1><h2>" +
            tr("Configuration ID:") + " %2<br>" + tr("Price:") + " %3</h2></font></p>"
            "<h3><table><caption>" + tr("Parameters:") + "</caption>%4</table></h3></body>");

    Solution* solution = solutions_[index];
    QStringList params = solution->fullDescription().toStringList();
    QString table;
    for(const QString& param : params)
    {
        QStringList tmp = param.split(':');
        QString paramName = tmp.first().simplified();
        QString paramValue = param.mid(paramName.size() + 2);

        table += "<tr><td>";
        table += paramName;
        table += "</td><td>";
        table += paramValue;
        table += "</td></tr>";
    }

    QTextDocument document;
    document.setHtml(htmlTemplate.arg(solution->mark() + " " + solution->model())
                                 .arg(solution->hash().toUpper().constData())
                                 .arg(QLocale().toCurrencyString(solution->price().getAsInteger()))
                                 .arg(table));

    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(file.toLocalFile());

    document.print(&printer);
    return true;
}

bool SolutionModel::printSolution(int index)
{
    if(index < 0 || index >= solutions_.size())
    {
        lastError_ = tr("Cannot print configuration: incorrect index %1").arg(index);
        return false;
    }

    QPrinter printer;
    QPrintDialog dlg(&printer);
    dlg.setOption(QAbstractPrintDialog::PrintToFile, false);
    dlg.setOption(QAbstractPrintDialog::PrintSelection, false);
    dlg.setOption(QAbstractPrintDialog::PrintPageRange, false);
    dlg.setOption(QAbstractPrintDialog::PrintCollateCopies, false);
    if(dlg.exec() == QDialog::Accepted)
    {
        static QString htmlTemplate =
        QString("<p align=\"center\"><h1>%1</h1><h2>" +
                tr("Configuration ID:") + " %2<br>" + tr("Price:") + " %3</h2></p>"
                "<h3><table><caption>" + tr("Parameters:") + "</caption>%4</table></h3>");

        Solution* solution = solutions_[index];
        QStringList params = solution->fullDescription().toStringList();
        QString table;
        for(const QString& param : params)
        {
            QStringList tmp = param.split(':');
            QString paramName = tmp.first().simplified();
            QString paramValue = param.mid(paramName.size() + 2);

            table += "<tr><td>";
            table += paramName;
            table += "</td><td>";
            table += paramValue;
            table += "</td></tr>";
        }

        QTextDocument document;
        document.setHtml(htmlTemplate.arg(solution->mark() + " " + solution->model())
                                     .arg(solution->hash().toUpper().constData())
                                     .arg(QLocale().toCurrencyString(solution->price().getAsInteger()))
                                     .arg(table));
        document.print(&printer);
    }
    return true;
}

bool SolutionModel::load(const QUrl& file)
{
    outdated_ = false;

    SolutionModel* tempModel = utils::XmlParser::instance()->loadSolutions(file.toLocalFile(), &outdated_);
    lastError_ = utils::XmlParser::instance()->lastError();
    if(tempModel)
    {
        restore();
        tempModel_ = tempModel;

        beginResetModel();
        tempMode_ = true;
        endResetModel();

        connect(tempModel_, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)), SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)));
        connect(tempModel_, SIGNAL(columnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)), SIGNAL(columnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
        connect(tempModel_, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)), SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)));
        connect(tempModel_, SIGNAL(columnsInserted(QModelIndex,int,int)), SIGNAL(columnsInserted(QModelIndex,int,int)));
        connect(tempModel_, SIGNAL(columnsMoved(QModelIndex,int,int,QModelIndex,int)), SIGNAL(columnsMoved(QModelIndex,int,int,QModelIndex,int)));
        connect(tempModel_, SIGNAL(columnsRemoved(QModelIndex,int,int)), SIGNAL(columnsRemoved(QModelIndex,int,int)));
        connect(tempModel_, SIGNAL(dataChanged(QModelIndex,QModelIndex)), SIGNAL(dataChanged(QModelIndex,QModelIndex)));
        connect(tempModel_, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)));
        connect(tempModel_, SIGNAL(headerDataChanged(Qt::Orientation,int,int)), SIGNAL(headerDataChanged(Qt::Orientation,int,int)));
        connect(tempModel_, SIGNAL(layoutAboutToBeChanged()), SIGNAL(layoutAboutToBeChanged()));
        connect(tempModel_, SIGNAL(layoutAboutToBeChanged(QList<QPersistentModelIndex>)), SIGNAL(layoutAboutToBeChanged(QList<QPersistentModelIndex>)));
        connect(tempModel_, SIGNAL(layoutAboutToBeChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)), SIGNAL(layoutAboutToBeChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)));
        connect(tempModel_, SIGNAL(layoutChanged()), SIGNAL(layoutChanged()));
        connect(tempModel_, SIGNAL(layoutChanged(QList<QPersistentModelIndex>)), SIGNAL(layoutChanged(QList<QPersistentModelIndex>)));
        connect(tempModel_, SIGNAL(layoutChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)), SIGNAL(layoutChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)));
        connect(tempModel_, SIGNAL(modelAboutToBeReset()), SIGNAL(modelAboutToBeReset()));
        connect(tempModel_, SIGNAL(modelReset()), SIGNAL(modelReset()));
        connect(tempModel_, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)), SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)));
        connect(tempModel_, SIGNAL(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)), SIGNAL(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
        connect(tempModel_, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)));
        connect(tempModel_, SIGNAL(rowsInserted(QModelIndex,int,int)), SIGNAL(rowsInserted(QModelIndex,int,int)));
        connect(tempModel_, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)));
        connect(tempModel_, SIGNAL(rowsRemoved(QModelIndex,int,int)), SIGNAL(rowsRemoved(QModelIndex,int,int)));

        return true;
    }
    else
        return false;
}

bool SolutionModel::isOutdated() const
{
    return outdated_;
}

void SolutionModel::restore()
{
    if(tempMode_)
    {
        tempMode_ = false;
        delete tempModel_;
        tempModel_ = nullptr;
        lastError_.clear();
    }
}

QString SolutionModel::lastError() const
{
    return lastError_;
}

void SolutionModel::clear()
{
    beginResetModel();
    qDeleteAll(solutions_);
    solutionsHash_.clear();
    solutions_.clear();
    delete tempModel_;
    tempMode_ = false;
    sortOrder_ = -1;
    tempModel_ = nullptr;
    lastError_.clear();
    endResetModel();
}

QHash<int,QByteArray> SolutionModel::roleNames() const
{
    if(tempMode_)
        return tempModel_->roleNames();

    Q_ASSERT(!roles_.isEmpty());
    return roles_;
}

}
}
