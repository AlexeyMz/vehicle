#pragma once

#include <QtCore/QAbstractListModel>
#include "nodeitem.h"

namespace vehicle {
namespace middleware {

class Parameter : public QObject
{
    Q_OBJECT
    Q_ENUMS(Type)
    Q_PROPERTY(Type type READ type CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QStringList list READ list CONSTANT)

public:
    enum Type {
        UnknownType = -1,
        ListType,
        BooleanType
    };

    explicit Parameter(Parameter* parent, AOTree::node_t* node);

    inline const Parameter* const parent() const { return parent_; }

    inline void setParentValue(const QString& value) { Q_ASSERT(parentValue_.isEmpty()); parentValue_ = value; }
    inline QString parentValue() const { return parentValue_; }

    void chooseValue(const QString& value);
    void addValue(const QString& value);
    QString value() const;

    inline void setName(const QString& name) { Q_ASSERT(name_.isEmpty()); name_ = name; }
    inline QString name() const { return name_; }
    inline Type type() const { return type_; }
    inline QStringList list() const { return QStringList() << " " << list_.keys(); }

    inline bool isVisible() const { return visible_; }

signals:
    void visibilityChanged();
    void valueChanged();

private:
    QString parentValue_;
    Parameter* parent_;
    bool visible_;

    QMap<QString,int> list_;
    AOTree::node_t* node_;
    QString value_;
    QString name_;
    Type type_;
};

template<typename Stream>
Stream& operator<<(Stream& os, const Parameter& p)
{
	os << "Parameter [" << p.name().toLocal8Bit().data() << "] = {";
    for(const QString& str : p.list())
        os << str.toLocal8Bit().data() << " ";
    os << "}" << std::endl;
    if(p.parent())
    {
        os << "Parent [" << p.parent()->name().toLocal8Bit().data();
        os << "] with value [" << p.parentValue().toLocal8Bit().data() << "]" << std::endl;
    }
    os << std::endl;

	return os;
}

class SolutionModel;

class ParameterModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ParameterRoles {
        ParameterRole = Qt::UserRole + 1,
        NameSizeRole,
    };

    explicit ParameterModel(AOTree* tree, QObject* parent = 0);
    ~ParameterModel();

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    void clear();

	const Parameter* const parameterAt(int pos) const;
    int parametersCount() const;

    SolutionModel* solutionModel() const;

public slots:
    void setParameterValue(const QString& name, const QString& value);

protected:
    QHash<int,QByteArray> roleNames() const;
    void addParameter(Parameter* parameter);

private:
    SolutionModel* solutionModel_;

    QVector<Parameter*> actualParams_;
    QVector<Parameter*> model_;

    QHash<int,QByteArray> roles_;
    AOTree* tree_;
    int nameSize_;
};

template<typename Stream>
Stream& operator<<(Stream& os, const ParameterModel& m)
{
	os << "{" << std::endl;
	for(int i = 0; i < m.parametersCount(); ++i)
		os << *m.parameterAt(i);
    os << "}" << std::endl;
	
	return os;
}

} // namespace middleware
} // namespace vehicle
