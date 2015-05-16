#pragma once

#include "decimal_for_cpp/decimal.h"

#include "datamodel/SolutionIterator.hpp"
#include "datamodel/AndOrTree.hpp"

namespace vehicle {
namespace middleware {

///
/// \brief Содержимое узла дерева.
/// Содержит название компонента комплектации автомобиля
/// (это может быть как "модель", так и, например, "двигатель").
/// \note Цена данного компонента содержится в дереве в качестве ключа.
/// "Фиксированность" элемента можно изменить в любой момент для пересчета
/// множества альтернатив, название задается лишь однажды при создании.
///
class NodeItem
{
public:
    explicit NodeItem(std::string name, bool fixed = false)
        : name_(name), fixed_(fixed) {}

    ///
    /// \return Название компонента
    ///
    inline std::string name() const { return name_; }
    ///
    /// \brief Изменение названия компонента
    /// \param name - новое название
    ///
    inline void setName(const std::string& name) { name_ = name; }
    ///
    /// \brief Метод необходим для поиска вариантов конфигураций
    /// \return "Зафиксирован ли выбор на данном узле среди альтернатив?"
    ///
    inline bool isFixed() const { return fixed_; }
    ///
    /// \brief Устанавливает "фиксированность" компонента
    /// \param fixed: true, если зафикисирован, иначе - false
    ///
    inline void setFixed(bool fixed) { fixed_ = fixed; }

private:
    std::string name_;
    bool fixed_;
};

template<typename Stream>
Stream& operator<<(Stream& os, const NodeItem& value)
{
    os << "\"" << value.name() << "\"";
    if(value.isFixed())
        os << " *";
    return os;
}

/// Тип И-ИЛИ дерева с ключом в виде десятичного числа с 2 знаками после запятой.
typedef core::AndOrTree<decimal2, NodeItem> AOTree;
/// Тип итератора подходящих конфигураций
typedef algorithm::SolutionIterator<typename AOTree::key_t, typename AOTree::value_t> solution_iterator;

} // namespace middleware
} // namespace vehicle
