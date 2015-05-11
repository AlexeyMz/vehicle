#pragma once

#include <QtCore/QAbstractListModel>
#include "nodeitem.h"

namespace vehicle {
namespace middleware {

///
/// \class Parameter
/// \brief Класс, представляющий собой параметр в
/// модели параметров \c ParameterModel. При добавлении
/// значений \see addValue() автоматически устанавливается
/// тип параметра \see Parameter::Type, который может быть
/// либо ListType (отображается в виде ComboBox, при этом
/// список значений возвращает функция list()), либо
/// BooleanType (отображается в виде CheckBox; данный тип
/// устанавливается если для данного параметра существует
/// ТОЛЬКО для значения: "Yes" или "No" / "Да" или "Нет"
/// после локализации). Параметр может зависить от другого
/// параметра (родителя \see parent()), тогда значение другого
/// параметра, при котором данный параметр будет активен,
/// возвращает функция parentValue(). Все "зависимые" параметры
/// соединены со своими "родителями" сигналом valueChanged(),
/// который вызывается каждый раз при выборе значения
/// параметра \see chooseValue()
///
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

    ///
    /// \brief Возвращает указатель на параметр, от значения которого зависит текущий
    /// \return Константный не изменяемый указатель на родителя
    ///
    inline const Parameter* const parent() const { return parent_; }

    ///
    /// \brief Устанавливает значение родительского параметра,
    /// при котором данный параметр становится видимым
    /// \param value - необходимое значение
    /// \note Вызвать данную функцию можно только один раз (при инициализации)
    ///
    inline void setParentValue(const QString& value) { Q_ASSERT(parentValue_.isEmpty()); parentValue_ = value; }
    ///
    /// \brief Возвращает необходимое значение родительского параметра
    ///
    inline QString parentValue() const { return parentValue_; }

    ///
    /// \brief Устанавливает текущее значение параметра,
    /// в модели данных, узел \c Node с таким значением
    /// устанавливается фиксированным \see NodeItem::setFixed, при этом
    /// со всех остальных sibling элементов флаг фиксированности
    /// снимается. Если выбранное значение отличается от текущего,
    /// посылается сигнал valueChanged()
    /// \param value - значение параметра
    ///
    void chooseValue(const QString& value);
    ///
    /// \brief Добавляет значение в список возможных значений параметра.
    /// В этой функции автоматически происходит определение типа \see type() параметра
    /// \param value - добавляемое значение
    ///
    void addValue(const QString& value);
    ///
    /// \brief Возвращает текущее выбранное значение параметра
    ///
    QString value() const;

    ///
    /// \brief Устанавливает имя параметра
    /// \param name - имя
    /// \note Имя не должно быть пустым
    ///
    inline void setName(const QString& name) { Q_ASSERT(name_.isEmpty()); name_ = name; }
    ///
    /// \brief Возвращает имя параметра
    ///
    inline QString name() const { return name_; }
    ///
    /// \brief Возвращает тип параметра
    /// \note Установить тип вручную нельзя, определение типа
    /// происходит при добавлении значений \see addValue()
    ///
    inline Type type() const { return type_; }
    ///
    /// \brief Возвращает список возможных значений параметра
    /// \note К списку добавляется также пустое значение, т.к.
    /// текущее значение параметра не обязательно должно быть задано
    ///
    inline QStringList list() const { return QStringList() << " " << list_.keys(); }

    ///
    /// \brief Видимость параметра определяется на основе значения
    /// родительского параметра \see parent() \see parentValue()
    /// \return true, если параметр виден и должен быть отображен в GUI
    ///
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
class TreeModel;

///
/// \class ParameterModel
/// \brief Класс, представляющий собой модель параметров
/// в виде списка. Базовый класс - QAbstractListModel.
/// Используется в качестве модели для стандартного представления
/// QML - ListView. Данный класс хранит в себе исходную модель данных
/// \c AndOrTree, модель решений \c SolutionModel, модель дерева для
/// визуального редактирования \c TreeModel и предоставляет непосредственно
/// доступ к режиму визуального редактирования данных модели \see openEditMode()
///
class ParameterModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ParameterRoles {
        ParameterRole = Qt::UserRole + 1,
        NameSizeRole,
    };

    ///
    /// \param tree - исходная модель данных, на основе которой происходит
    /// инициализация всех остальных моделей \see initialize()
    /// \note ParameterModel берет \p tree под свой контроль
    /// \note Указатель \p tree должен быть валидным
    ///
    explicit ParameterModel(AOTree* tree, QObject* parent = 0);
    ~ParameterModel();

    ///
    /// \brief Определение чисто виртуального метода базового класса
    /// \return Количество строк в модели
    ///
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    ///
    /// \brief Определение чисто виртуального метода базового класса
    /// \param index - cтрока индекса должна быть >= 0 и < кол-ва строка \see rowCount()
    /// \param role - может быть любым значением перечисления \e ParameterRoles
    /// \return Возвращает данные по индексу \p index для заданной роли \p role
    ///
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    ///
    /// \brief Вовзращает константный не изменяемый указатель
    /// на параметр \c Parameter на заданной позиции
    /// \param pos - позиция требуемого параметра
    /// \note Номер строки \p pos должен быть >= 0 и < кол-ва параметров \see parametersCount()
    /// \note При поиске "видимость" параметра не учитывается
    ///
	const Parameter* const parameterAt(int pos) const;
    ///
    /// \brief Возвращает количество всех параметров в модели
    /// (внезависимости от их "видимости" \see Parameter::isVisible())
    ///
    int parametersCount() const;

    ///
    /// \brief Возвращает модель решений \c SolutionModel
    ///
    SolutionModel* solutionModel() const;

    ///
    /// \brief Открывает режим визуального редактирования
    /// даннных модели, представленной моделью дерева \c TreeModel
    /// \note Метод вызывается из QML
    ///
    Q_INVOKABLE void openEditMode();

public slots:
    ///
    /// \brief Устанавливается текущее значение заданного параметра \see Parameter::chooseValue()
    /// \param name - имя параметра, значение которого необходимо установить
    /// \param value - новое значение параметра
    /// \note Метод вызывается из QML при изменении значений параметров
    ///
    void setParameterValue(const QString& name, const QString& value);

private slots:
    void treeChanged();

protected:
    QHash<int,QByteArray> roleNames() const;
    void addParameter(Parameter* parameter);
    void initialize();
    void clear();

private:
    SolutionModel* solutionModel_;
    TreeModel* treeModel_;

    QVector<Parameter*> actualParams_;
    QVector<Parameter*> model_;

    QHash<int,QByteArray> roles_;
    AOTree* tree_;
    int nameSize_;
    bool changed_;
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
