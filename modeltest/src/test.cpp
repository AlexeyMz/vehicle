#include <iostream>
#include <assert.h>
#include "decimal_for_cpp/decimal.h"

#include "AndOrTree.hpp"
#include "SolutionIterator.hpp"

using namespace vehicle::core;
using namespace vehicle::algorithm;

/**
 * Содержимое узла дерева.
 * Содержит всю информацию о компоненте автомобиля, кроме цены.
 * Метод isFixed() необходим для поиска вариантов конфигураций и возвращает
 * значение "зафиксирован ли выбор на данном узле среди альтернатив?".
 */
class ItemValue {
public:
    explicit ItemValue(std::string title, bool fixed = false):
        title(title),
        fixed(fixed) {}

    bool isFixed() const { return fixed; }

    std::string title;
    bool fixed;
};

template <typename Stream>
Stream & operator<<(Stream &os, const ItemValue &value) {
    os << "\"" << value.title << "\"";
    if (value.isFixed()) { os << " *"; }
    return os;
}

/** Тип И-ИЛИ дерева с ключом в виде десятичного числа с 2 знаками после запятой. */
typedef AndOrTree<decimal2, ItemValue> AOTree;

int main() {
    // создание и инициализация дерева
    AOTree tree;
    tree.setRoot(tree.create(NodeKind::AND, decimal2(42), ItemValue("hello"))
        ->append(NodeKind::NONE, decimal2(10), ItemValue("world"))
        ->attach(tree.create(NodeKind::OR, decimal2(33), ItemValue("foo"))
            ->append(NodeKind::NONE, decimal2(44), ItemValue("baz"))
            ->append(NodeKind::NONE, decimal2(55), ItemValue("quax"))
            ->attach(tree.create(NodeKind::OR, decimal2(1), ItemValue("frob"))
                ->append(NodeKind::NONE, decimal2(5), ItemValue("crux"))
                ->append(NodeKind::NONE, decimal2(2), ItemValue("xell"))))
        ->append(NodeKind::NONE, decimal2(77), ItemValue("bar"))
        ->attach(tree.create(NodeKind::OR, decimal2(3), ItemValue("zyx"))
            ->append(NodeKind::NONE, decimal2(11), ItemValue("xyzzy"))
            ->append(NodeKind::NONE, decimal2(12), ItemValue("sel", true))
            ->append(NodeKind::NONE, decimal2(13), ItemValue("nonsel"))));

    // обращение к корневому узлу дерева
    AOTree::node_t *n = tree.getRoot();
	assert(n->subtreeKey() == decimal2(179));

    // копирование дерева
    auto copy = tree;
    // изменение ключа узла
    copy.getRoot()->child(1)->setOwnKey(decimal2(1000));
    // вывод в поток
    std::cout << copy;
	assert(copy.getRoot()->subtreeKey() == decimal2(1146));

    // итератор подходящих конфигураций
    typedef SolutionIterator<
        typename AOTree::key_t,
        typename AOTree::value_t> solution_iterator;
    solution_iterator iter(copy);

	assert(iter.solutionCount() == 4);
    std::cout
        << "\nEnumerating " << iter.solutionCount()
        << " solutions: <press enter>" << std::endl;
    std::string s;
    std::getline(std::cin, s);

    size_t solutionNumber = 1;
    do {
        // вывод очередного решения;
        // решение представлено в виде дерева узлов,
        // у которых те же ключи и другой тип значения:
        // Choice (см. SolutionIterator.hpp)
        std::cout << "@ solution " << solutionNumber++ << "\n"
                  << iter.currentSolution();
		assert(iter.currentSolution().getRoot()->getValue().node == copy.getRoot());
        // пример работы с решением - см. operator<< для Node<K, Choice<K, V>>
        std::getline(std::cin, s);
    } while (iter.nextSolution());

    std::cout << "All solutions enumerated." << std::endl;
    std::getline(std::cin, s);

	return 0;
}
