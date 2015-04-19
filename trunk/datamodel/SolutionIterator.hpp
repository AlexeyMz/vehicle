#pragma once

#include <assert.h>
#include <vector>
#include <algorithm>
#include <numeric>

#include "AndOrTree.hpp"

namespace vehicle {
    namespace algorithm {
        using namespace core;

        template <typename Key, typename Value>
        struct Choice {
            typedef Node<Key, Value> node_t;
            Choice(const node_t *node, bool isFixed, size_t index):
                node(node),
                hasChoice(true),
                isFixed(isFixed),
                index(index),
                power(0) {}
            Choice(const node_t *node):
                node(node),
                hasChoice(false),
                isFixed(false),
                index(0),
                power(0) {}
            /** Узел исходного дерева. */
            const node_t * const node;
            /**
             * Значение "возможен ли выбор альтернативного варианта
             * среди детей данного узла?" (т.е. узел имеет тип ИЛИ
             * и у него есть дети).
             * Данное значение не зависит от значения isFixed.
             */
            const bool hasChoice;
            /**
             * Значение "Выбран (зафиксирован) ли даный узел среди альтернатив?"
             * Узел считается зафиксированным, если истинно node->getValue().isFixed()
             */
            const bool isFixed;
            /**
             * Индекс выбранной среди непосредственный детей узла альтернативы.
             * Если среди детей есть зафиксированный узел (isFixed), то
             * значение будет равно его индексу в списке детей узла.
             */
            size_t index;
            /**
             * Мощность поддерева альтернатив, с корнем в данном узле.
             * Означает количество решений, которые можно построить для поддерева,
             * с учётом зафиксированных узлов.
             */
            size_t power;
        };

        template <typename Key, typename Value>
        Key choiceBasedComputeKey(Node<Key, Choice<Key, Value>> &node) {
            typedef Node<Key, Choice<Key, Value>> node_t;
            auto &choice = node.getValue();
            Key key = node.ownKey();
            if (choice.hasChoice) {
                if (choice.index < node.childCount()) {
                    key = key + node.child(choice.index)->subtreeKey();
                }
            } else {
                for (auto child : node) {
                    key = key + child->subtreeKey();
                }
            }
            return key;
        }

        /**
         * Итератор для поиска всех возможных альтернатив И-ИЛИ дерева
         * при выборе одного из узлов-потомков для каждого из ИЛИ-узла.
         */
        template <typename Key, typename Value>
        class SolutionIterator {
        public:
            typedef AndOrTree<Key, Value> tree_t; 
            typedef typename tree_t::node_t node_t;

            typedef Choice<Key, Value> choice_t;
            typedef AndOrTree<Key, choice_t> solution_tree_t;
            typedef typename solution_tree_t::node_t solution_node_t;

            SolutionIterator(const tree_t &source):
                source(source),
                solution(&choiceBasedComputeKey<Key, Value>)
            {
                solution.setRoot(deepCloneNodeForSolution(source.getRoot()));
            }

            size_t solutionCount() {
                return solution.getRoot()->getValue().power;
            }

            bool nextSolution() {
                Switch sw = nextChoice(solution.getRoot());
                return sw == Success;
            }

            const solution_tree_t & currentSolution() const {
                return solution;
            }

        private:
            enum Switch { None, Success, Overflow };

            bool hasChoice(const node_t *node) {
                return node->getKind() == NodeKind::or && !node->isLeaf();
            }

            solution_node_t * deepCloneNodeForSolution(const node_t *node) {
                bool choiceExists = hasChoice(node);
                auto solutionNode = solution.create(
                    node->getKind(), node->ownKey(),
                    choiceExists ? createChoice(node) : choice_t(node));
                for (auto child : *node) {
                    solutionNode->attach(deepCloneNodeForSolution(child));
                }
                recomputePower(solutionNode);
                return solutionNode;
            }

            choice_t createChoice(const node_t *parent) {
                assert(parent && hasChoice(parent));
                auto begin = parent->begin();
                auto end = parent->end();
                auto it = std::find_if(begin, end, [] (const node_t *child) {
                    return child->getValue().isFixed();
                });
                if (it == end) {
                    return choice_t(parent, false, 0);
                } else {
                    return choice_t(parent, true, it - begin);
                }
            }

            Switch nextChoice(solution_node_t *node) {
                if (node->isLeaf()) { return None; }
                auto &choice = node->getValue();
                Switch sw = None;
                if (choice.hasChoice) {
                    auto choiceIndex = choice.index;
                    sw = nextChoice(node->child(choiceIndex));
                    if (sw == Success) { return sw; }
                    else {
                        if (choice.isFixed) {
                            sw = Overflow;
                        } else if (choiceIndex < node->childCount() - 1) {
                            choice.index++;
                            sw = Success;
                        } else {
                            choice.index = 0;
                            sw = Overflow;
                        }
                        solution.recomputeKey(node);
                    }
                } else {
                    for (auto child : *node) {
                        Switch childSwitch = nextChoice(child);
                        if (childSwitch != None) { sw = childSwitch; }
                        if (childSwitch == Success) { break; }
                    }
                }
                return sw;
            }

            void recomputePower(solution_node_t *node) {
                auto &choice = node->getValue();
                if (choice.hasChoice) {
                    if (choice.isFixed) {
                        auto choosen = node->child(choice.index);
                        choice.power = choosen->getValue().power;
                    } else {
                        choice.power = std::accumulate(node->begin(), node->end(), 0,
                            [] (size_t acc, solution_node_t *n) { return acc + n->getValue().power; });
                    }
                } else {
                    choice.power = std::accumulate(node->begin(), node->end(), 1,
                        [] (size_t acc, solution_node_t *n) { return std::max(acc, n->getValue().power); });
                }
            }

            const tree_t &source;
            solution_tree_t solution;
        };

        template <typename Stream, typename Key, typename Value>
        Stream & operator<<(Stream &os, const Choice<Key, Value> &choice) {
            return os << choice.node->getValue();
        }

        namespace {
            template <typename Stream, typename Key, typename Value>
            Stream & print(Stream &stream, const Node<Key, Choice<Key, Value>> &node, size_t level) {
                typedef Choice<Key, Value> choice_t;
                design::printNode(stream, node, level);
                
                const choice_t &choice = node.getValue();
                if (choice.hasChoice) {
                    print(stream, *node.child(choice.index), level + 1);
                } else {
                    for (auto child : node) {
                        print(stream, *child, level + 1);
                    }
                }

                if (!node.isLeaf()) {
                    design::closeLevels(stream, "  ", "]\n", level + 1, level);
                }
                return stream;
            }
        }

        template <typename Stream, typename Key, typename Value>
        Stream & operator<<(Stream &os, const Node<Key, Choice<Key, Value>> &node) {
            print(os, node, 0);
            return os;
        }
    }
}
