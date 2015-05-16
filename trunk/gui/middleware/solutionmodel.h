#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QFutureWatcher>
#include <QtCore/QUrl>

#include "nodeitem.h"

namespace vehicle {
namespace middleware {

namespace internal {
///
/// \struct SolutionInitializer
/// \brief Структура для инициализации решения \c Solution
/// собственными данными, без использования итератора
/// по решениям \c SolutionIterator
///
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

///
/// \class Solution
/// \brief Класс, представляющий собой решение в
/// модели решений \c SolutionModel. Может быть инициализирован
/// тремя способами: 1) передачей в конструктор поддерева решений
/// из итератора по решениям \see SolutionIterator::currenctSolution();
/// 2) передачей в конструктор структуры инициализации \see SolutionInitializer
/// с собственным данными; и 3) путем копирования решения с использованием
/// конструктора копирования. После создания экземпляра данного класса и его
/// инициализации - данные изменить нельзя, класс работает только на чтение.
///
class Solution
{
public:
    explicit Solution(solution_iterator::solution_tree_t solution);
    Solution(internal::SolutionInitializer&& solution);
    Solution(const Solution& solution);

    ///
    /// \brief Возвращает список параметров конфигурации
    /// в виде строк "%Name%: %Value% (%Price%)"
    /// \return QStringList
    /// \note Список строк используется в качестве
    /// модели данных для ListView в QML
    /// \note Если %Price% == 0, то она не отображается
    ///
    inline QVariant fullDescription() const { return QVariant::fromValue(data_.fullDescription); }
    ///
    /// \brief Возвращает краткое описание параметров конфигурации,
    /// которые отображаются в главном списке.
    /// \note Краткое описание получается путем конкатенации всех
    /// строк из списка параметров \see fullDescription() и обрезкой
    /// получившейся строки до 500 пикселей (с добавлением "..." справа,
    /// если длина получившейся строки превышала максимальную)
    ///
    inline QString shortDescription() const { return data_.shortDescription; }
    ///
    /// \brief Возвращает общую стоимость конфигурации
    ///
    inline AOTree::key_t price() const { return data_.price; }
    ///
    /// \brief Возвращает наименование модели
    /// для которой составлена данная конфигурация
    ///
    inline QString model() const { return data_.model; }
    ///
    /// \brief Возвращает наименование марки
    /// для которой составлена данная конфигурация
    ///
    inline QString mark() const { return data_.mark; }
    ///
    /// \brief Возвращает идентификатор данной конфигурации
    ///
    inline QByteArray hash() const { return data_.hash; }

private:
    void initialize(solution_iterator::solution_tree_t solution);

    internal::SolutionInitializer data_;
};

///
/// \class SolutionModel
/// \brief Класс, представляющий собой модель решений в виде списка.
/// Базовый класс - QAbstractListModel. Используется в качестве модели
/// для стандартного представления QML - ListView. Данный класс предоставляет
/// возможность сохранения текущих конфигураций \see save(), открытия сохраненных
/// конфигураций \see load(), вывода на печать и в PDF текущей конфигурации
/// \see saveSolution(), \see printSolution().
/// При загрузке конфигураций \see load(), модель переходит во "временный режим",
/// в котором создается еще одна модель-делегат и все функции перенаправляются
/// этой модели, до тех пор, пока не будет вызвана функция восстановления \see restore()
///
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

    ///
    /// \brief Статическая функция, предназначения для
    /// генерации модели решений на основе итератора по решениям
    /// \param solutions - итератор по решениям
    ///
    static SolutionModel* create(solution_iterator solutions, QObject* parent = 0);

    explicit SolutionModel(QObject* parent = 0);
    ~SolutionModel();

    ///
    /// \brief Делает текущую модель копией переданной модели,
    /// удаляя решения, отсутствующие в переданной модели и
    /// добавляя те, которые отсутствуют в текущей
    /// \param model - модель, копией которой станет
    /// текущая модель, после выполнения данной функции
    /// \note \p model должен быть валидным указателем
    /// \note модель \p model не должна быть отсортирована
    ///
    void recomputeToFit(SolutionModel* model);

    ///
    /// \brief Определение чисто виртуального метода базового класса
    /// \return Количество строк в модели
    ///
    Q_INVOKABLE int rowCount(const QModelIndex& parent = QModelIndex()) const;
    ///
    /// \brief Определение чисто виртуального метода базового класса
    /// \param column - колонка, по которой производится сортировка,
    /// т.к. модель - это список, column должна быть равна 0
    /// \param order - тип сортировки
    /// \return Выполняет сортировку модели в соответствии с \p order
    ///
    Q_INVOKABLE void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
    ///
    /// \brief Определение чисто виртуального метода базового класса
    /// \param index - cтрока индекса должна быть >= 0 и < кол-ва строка \see rowCount()
    /// \param role - может быть любым значением перечисления \e ParameterRoles
    /// \return Возвращает данные по индексу \p index для заданной роли \p role
    ///
    Q_INVOKABLE QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    ///
    /// \brief Определение чисто виртуального метода базового класса
    /// \param row - строка
    /// \param column - столбец
    /// \return Сгенерированный индекс для строки \p row и столбца \p column
    ///
    Q_INVOKABLE QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

    ///
    /// \brief Добавление решения в модель
    /// \param solution - решение для добавления
    ///
    void addSolution(Solution* solution);
    ///
    /// \brief Очистка модели (возвращение ее к состоянию на момент создания)
    ///
    void clear();

    ///
    /// \brief Сохранение конфигураций в файл \p file
    /// \param file - путь к файлу
    /// \return false, в случае, если сохранение не удалось \see lastError()
    ///
    Q_INVOKABLE bool save(const QUrl& file);
    ///
    /// \brief Загрузка конфигураций из файла \p file
    /// \param file - путь к файлу
    /// \return false, в случае, если загрузка не удалась \see lastError()
    ///
    Q_INVOKABLE bool load(const QUrl& file);
    ///
    /// \brief После загрузки конфигураций \see load(),
    /// возвращает true, если загруженные конфигурации
    /// являются устаревшими (т.е. были созданы на основе
    /// устаревших данных)
    ///
    Q_INVOKABLE bool isOutdated() const;

    ///
    /// \brief Сохранение конкретной конфигурации в файл \p file (PDF)
    /// \param index - индекс конфигурации в модели
    /// \param file - имя файла в формате PDF
    /// \return false, в случае, если сохранение не удалось \see lastError()
    ///
    Q_INVOKABLE bool saveSolution(int index, const QUrl& file);
    ///
    /// \brief Печать конкретной конфигурации
    /// \param index - индекс конфигурации в модели
    /// \return false, в случае, если печать не удалась \see lastError()
    ///
    Q_INVOKABLE bool printSolution(int index);

    ///
    /// \brief Возвращает описание последней возникшей ошибки
    ///
    Q_INVOKABLE QString lastError() const;

    ///
    /// \brief Возвращает модель из временого режима,
    /// после загрузки конфигураций \see load()
    ///
    Q_INVOKABLE void restore();

signals:
	///
	/// \brief Данный сигнал отправляется при вызове функции \fn sort()
	/// перед выполнением сортировки. Сигнал обрабатывается в QML
	///
	void sortingStarted();
	///
	/// \brief Данный сигнал отправляется при вызове функции \fn sort()
	/// после выполнения сортировки. Сигнал обрабатывается в QML
	///
	void sortingFinished();

private slots:
	void startSorting(int column, Qt::SortOrder order);
	void endSorting();

protected:
    QHash<int,QByteArray> roleNames() const;

private:
    SolutionModel* tempModel_;
    QString lastError_;
    bool tempMode_;

    QHash<QString,Solution*> solutionsHash_;
	QFutureWatcher<void> sorting_;
    QVector<Solution*> solutions_;
    QHash<int,QByteArray> roles_;
    int sortOrder_;
    bool outdated_;
};

} // namespace middleware
} // namespace vehicle
