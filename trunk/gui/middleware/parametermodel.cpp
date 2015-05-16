#include <QtWidgets/QApplication>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>

#include <QtConcurrent/QtConcurrentRun>
#include <QtGui/QFontMetrics>

#include <QtCore/QCryptographicHash>
#include <QtCore/QSettings>
#include <QtCore/QTime>

#include "../utils/xmlparser.h"

#include "treeview.h"
#include "treemodel.h"
#include "solutionmodel.h"
#include "parametermodel.h"

namespace vehicle {
using namespace algorithm;
namespace middleware {

Parameter::Parameter(Parameter* parent, AOTree::node_t* node)
    : parent_(parent), node_(node), type_(UnknownType), visible_(!parent_)
{
    if(parent_)
    {
        connect(parent_, &Parameter::valueChanged, [=]()
        {
            if(parent_->value() == parentValue())
            {
                if(!visible_)
                {
                    visible_ = true;
                    emit visibilityChanged();
                }
            }
            else
            {
                if(visible_)
                {
                    visible_ = false;
                    emit visibilityChanged();
                }
            }
        });

        connect(parent_, &Parameter::visibilityChanged, [=]()
        {
            if(visible_ && !parent_->isVisible())
            {
                visible_ = false;
                emit visibilityChanged();
            }
        });

        connect(this, &Parameter::visibilityChanged, [=]()
        {
            if(!visible_)
            {
                value_.clear();
                for(size_t i = 0; i < node_->childCount(); ++i)
                    node_->child(i)->getValue().setFixed(false);
            }
        });
    }
}

void Parameter::chooseValue(const QString& value)
{
    if(value_ != value)
    {
        value_ = value;
        for(size_t i = 0; i < node_->childCount(); ++i)
        {
            auto child = node_->child(i);
            if(type_ == BooleanType && value.compare(tr("No"), Qt::CaseInsensitive) == 0)
                child->getValue().setFixed(false);
            else
                child->getValue().setFixed(child->getValue().name().c_str() == value);
        }
        emit valueChanged();
    }
}

void Parameter::addValue(const QString& value)
{
    if(type_ == ListType || type_ == UnknownType)
    {
        list_.insert(value, 0);
        type_ = ListType;
    }
    else if(type_ == BooleanType || type_ == UnknownType)
    {
        if(value.compare(tr("Yes")) != 0 && value.compare(tr("No")) != 0)
        {
            list_.insert(value, 0);
            type_ = ListType;
        }
    }
    else
        Q_UNREACHABLE();

    if(list_.size() == 2 && list_.contains(tr("Yes")) && list_.contains(tr("No")))
        type_ = BooleanType;
}

QString Parameter::value() const
{
    return value_;
}

ParameterModel::ParameterModel(AOTree* tree, QObject* parent) : QAbstractListModel(parent), solutionModel_(0), treeModel_(0), changed_(false), tree_(tree), nameSize_(-1)
{
    Q_ASSERT(tree_);
	connect(&paramSet_, SIGNAL(started()), SIGNAL(parameterSetStarted()));
	connect(&paramSet_, SIGNAL(finished()), SLOT(endUpdateSolutionModel()));
	connect(&paramSet_, SIGNAL(finished()), SIGNAL(parameterSetFinished()));

    initialize();

    roles_[ParameterRole] = "Parameter";
    roles_[NameSizeRole] = "NameWidth";

#ifdef _DEBUG
    std::cout << *this << std::endl;
#endif
}

ParameterModel::~ParameterModel()
{
	paramSet_.waitForFinished();
    delete solutionModel_;
    delete tree_;
    clear();
}

void ParameterModel::initialize()
{
    Q_ASSERT(!treeModel_);

    static std::function<void(Parameter*,const QString&,AOTree::node_t*,ParameterModel*)> expandParameter =
    [](Parameter* parent, const QString& parentValue, AOTree::node_t* node, ParameterModel* model)
    {
        // если разворачиваем узел ИЛИ
        if(node->getKind() == NodeKind::OR)
        {
            // создаем параметр с переданным родителем и текущим узлом
            Parameter* parameter = new Parameter(parent, node);
            parameter->setName(node->getValue().name().c_str());
            model->addParameter(parameter);

            // если родитель есть, устанавливаем его необходимое значение
            if(parent)
                parameter->setParentValue(parentValue);

            // обходим всех детей данного узла
            for(size_t i = 0; i < node->childCount(); ++i)
            {
                auto child = node->child(i);
                // добавляем в текущий узел возможное его значение
                parameter->addValue(child->getValue().name().c_str());
                // и разворачиваем дочерний узел с родителем в виде текущего параметра
                // и значением родителя в виде имени текущего дочерного узла
                expandParameter(parameter, child->getValue().name().c_str(), child, model);
            }
        }
        // если разворачиваем узел И
        else if(node->getKind() == NodeKind::AND)
            // обходим всех детей данного узла
            for(size_t i = 0; i < node->childCount(); ++i)
                // если у узла есть родитель и это И узел,
                // то разворачиваем этот узел с тем жи параметрами, что были переданы
                if(node->getParent() && node->getParent()->getKind() == NodeKind::AND)
                    expandParameter(parent, parentValue, node->child(i), model);
                // иначе, значение родителя устанавливаем в значение текущего узла
                else
                    expandParameter(parent, node->getValue().name().c_str(), node->child(i), model);
    };

    static std::function<void(AOTree::node_t*)> unfixNode =
    [](AOTree::node_t* node)
    {
        node->getValue().setFixed(false);
        for(size_t i = 0; i < node->childCount(); ++i)
            unfixNode(node->child(i));
    };

    auto root = tree_->getRoot();
    if(root)
    {
        unfixNode(root);
        expandParameter(nullptr, "", root, this);
    }

    solution_iterator it(*tree_);
    SolutionModel* solutionModel = SolutionModel::create(it, this);
    if(solutionModel_)
    {
        solutionModel_->clear();
        solutionModel_->recomputeToFit(solutionModel);
    }
    else
        solutionModel_ = solutionModel;

    treeModel_ = new TreeModel(tree_, this);
    connect(treeModel_, SIGNAL(dataChanged(QModelIndex,QModelIndex)), SLOT(treeChanged()));
    connect(treeModel_, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(treeChanged()));
    connect(treeModel_, SIGNAL(rowsRemoved(QModelIndex,int,int)), SLOT(treeChanged()));
}

void ParameterModel::treeChanged()
{
    changed_ = true;
}

int ParameterModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return actualParams_.count();
}

QVariant ParameterModel::data(const QModelIndex& index, int role) const
{
    if(index.row() < 0 || index.row() >= rowCount())
        return QVariant();

    switch(role)
    {
    case ParameterModel::ParameterRole :
        return QVariant::fromValue(actualParams_[index.row()]);
    case ParameterModel::NameSizeRole :
        return nameSize_;
    default :
        Q_UNREACHABLE();
        return QVariant();
    }
}

void ParameterModel::addParameter(Parameter* parameter)
{
    model_.push_back(parameter);
    if(parameter->isVisible())
        actualParams_.push_back(parameter);
    else
    {
        connect(parameter, &Parameter::visibilityChanged, [=]()
        {
            if(parameter->isVisible())
            {
                beginInsertRows(QModelIndex(), rowCount(), rowCount());
                actualParams_ << parameter;
                endInsertRows();
            }
            else
            {
                for(int i = 0; i < actualParams_.size(); ++i)
                {
                    if(actualParams_[i] == parameter)
                    {
                        beginRemoveRows(QModelIndex(), i, i);
                        actualParams_.remove(i);
                        endRemoveRows();
                        break;
                    }
                    Q_ASSERT(i != actualParams_.size() - 1);
                }
            }
        });
    }

    int width = QFontMetrics(QGuiApplication::font()).size(Qt::TextSingleLine, parameter->name()).width() + 10;
    if(width > nameSize_)
        nameSize_ = width;
    if(nameSize_ > 130)
        nameSize_ = 130;
}

const Parameter* const ParameterModel::parameterAt(int pos) const
{
	Q_ASSERT(pos >= 0 && pos < parametersCount());
    return model_.at(pos);
}

int ParameterModel::parametersCount() const
{
    return model_.count();
}

void ParameterModel::setParameterValue(const QString& name, const QString& value)
{
	paramSet_.waitForFinished();

    Parameter* parameter = nullptr;
    for(Parameter* p : actualParams_)
        if(p->name() == name)
        {
            parameter = p;
            break;
        }

    Q_ASSERT(parameter != nullptr);
    parameter->chooseValue(value);

	paramSet_.setFuture(QtConcurrent::run(this, &ParameterModel::startUpdateSolutionModel));
}

SolutionModel* ParameterModel::startUpdateSolutionModel()
{
	solution_iterator it(*tree_);
    return SolutionModel::create(it);
}

void ParameterModel::endUpdateSolutionModel()
{
	solutionModel_->recomputeToFit(paramSet_.result());
    delete paramSet_.result();
}

SolutionModel* ParameterModel::solutionModel() const
{
    return solutionModel_;
}

void ParameterModel::openEditMode()
{
    changed_ = false;

    QDialog* modalDialog = new QDialog;
    modalDialog->setWindowTitle(tr("The data edit mode"));
    modalDialog->setModal(true);

    modalDialog->setStyleSheet("QTreeView { show-decoration-selected: 1; }");

    TreeView* view = new TreeView(modalDialog);
    view->setModel(treeModel_);
    view->expandAll();

    for(int i = 0; i < treeModel_->columnCount(); ++i)
        view->resizeColumnToContents(i);

    QPushButton* changeButton = new QPushButton(tr("Change password"), view);
    connect(changeButton, &QPushButton::clicked, []()
    {
        bool ok;
        QString pwd = QInputDialog::getText(0, tr("Change password"), tr("Input new password"), QLineEdit::Password, QString(), &ok);

        if(ok)
        {
            if(pwd.isEmpty())
                QMessageBox::critical(0, tr("Error"), tr("You have entered an empty password!"));
            else
            {
                QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "Vehicle", "Settings");
                settings.setValue("password", QCryptographicHash::hash(pwd.toLocal8Bit(), QCryptographicHash::Md5));
            }
        }
    });

    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->addWidget(new QLabel(QString("<font color=\"#9e0000\">& - %1</font><br><font color=\"#0c3696\">|| - %2</font>").arg(tr("AND nodes")).arg(tr("OR nodes")), view));
    topLayout->addStretch();
    topLayout->addWidget(changeButton);


    QVBoxLayout* layout = new QVBoxLayout(modalDialog);
    layout->addLayout(topLayout);
    layout->addWidget(view);

    modalDialog->resize(modalDialog->width(), modalDialog->height());
    modalDialog->exec();

    if(changed_)
    {
        if(!utils::XmlParser::instance()->saveModel(tree_, qApp->applicationDirPath() + "/data.xml"))
            QMessageBox::warning(0, tr("Warning"), QString("%1 (%2)").arg(tr("Failed to save data")).arg(utils::XmlParser::instance()->lastError()));

        clear();
        beginResetModel();
        initialize();
        endResetModel();
    }
    delete modalDialog;
}

void ParameterModel::clear()
{
    beginResetModel();
    qDeleteAll(model_);
    model_.clear();
    actualParams_.clear();
    nameSize_ = 0;
    delete treeModel_;
    treeModel_ = 0;
    endResetModel();
}

QHash<int,QByteArray> ParameterModel::roleNames() const
{
    Q_ASSERT(!roles_.isEmpty());
    return roles_;
}

}
}
