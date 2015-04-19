#include <iostream>
#include <decimal_for_cpp/decimal.h>

#include "datamodel/AndOrTree.hpp"
#include "datamodel/SolutionIterator.hpp"

using namespace vehicle::core;
using namespace vehicle::algorithm;

/**
 * ���������� ���� ������.
 * �������� ��� ���������� � ���������� ����������, ����� ����.
 * ����� isFixed() ��������� ��� ������ ��������� ������������ � ����������
 * �������� "������������ �� ����� �� ������ ���� ����� �����������?".
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

/** ��� �-��� ������ � ������ � ���� ����������� ����� � 2 ������� ����� �������. */
typedef AndOrTree<dec::decimal2, ItemValue> AOTree;

int main() {
    // �������� � ������������� ������
    AOTree tree;
    tree.setRoot(tree.create(NodeKind::and, dec::decimal2(42), ItemValue("hello"))
        ->append(NodeKind::none, dec::decimal2(10), ItemValue("world"))
        ->attach(tree.create(NodeKind::or, dec::decimal2(33), ItemValue("foo"))
            ->append(NodeKind::none, dec::decimal2(44), ItemValue("baz"))
            ->append(NodeKind::none, dec::decimal2(55), ItemValue("quax"))
            ->attach(tree.create(NodeKind::or, dec::decimal2(1), ItemValue("frob"))
                ->append(NodeKind::none, dec::decimal2(5), ItemValue("crux"))
                ->append(NodeKind::none, dec::decimal2(2), ItemValue("xell"))))
        ->append(NodeKind::none, dec::decimal2(77), ItemValue("bar"))
        ->attach(tree.create(NodeKind::or, dec::decimal2(3), ItemValue("zyx"))
            ->append(NodeKind::none, dec::decimal2(11), ItemValue("xyzzy"))
            ->append(NodeKind::none, dec::decimal2(12), ItemValue("sel", true))
            ->append(NodeKind::none, dec::decimal2(13), ItemValue("nonsel"))));

    // ��������� � ��������� ���� ������
    AOTree::node_t *n = tree.getRoot();
    n->subtreeKey();

    // ����������� ������
    auto copy = tree;
    std::cout << copy;

    // �������� ���������� ������������
    typedef SolutionIterator<
        typename AOTree::key_t,
        typename AOTree::value_t> solution_iterator;
    solution_iterator iter(copy);

    std::cout
        << "\nEnumerating " << iter.solutionCount()
        << " solutions: <press enter>" << std::endl;
    std::string s;
    std::getline(std::cin, s);

    size_t solutionNumber = 1;
    do {
        // ����� ���������� �������;
        // ������� ������������ � ���� ������ �����,
        // � ������� �� �� ����� � ������ ��� ��������:
        // Choice (��. SolutionIterator.hpp)
        std::cout << "@ solution " << solutionNumber++ << "\n"
                  << iter.currentSolution();
        // ������ ������ � �������� - ��. operator<< ��� Node<K, Choice<K, V>>
        std::getline(std::cin, s);
    } while (iter.nextSolution());

    std::cout << "All solutions enumerated." << std::endl;
    std::getline(std::cin, s);

	return 0;
}
