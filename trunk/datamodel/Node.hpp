#pragma once

#include <assert.h>
#include <iostream>
#include <vector>

namespace vehicle {
	namespace core {
        /**
         * Тип узла И-ИЛИ дерева.
         */
		enum class NodeKind { AND, OR, NONE };

        template <typename Key, typename Value>
        class AndOrTree;

        /**
         * Узел И-ИЛИ дерева:
         *  - явно принадлежит конкретному дереву (owner);
         *  - находится в составе дерева, если он является корневым,
         *    либо поднимаясь по цепочке родителей (parent) верхний узел
         *    (у которого нет родителя) будет совпадать с корневым узлом дерева.
         */
		template <typename Key, typename Value>
		class Node /* final */ {
		public:
            typedef Key key_t;
			typedef Value value_t;
            typedef AndOrTree<Key, Value> tree_t;
            
            friend class tree_t;

        private:
            typedef std::vector<Node *> children_t;

        public:
            /**
             * Итератор обхода поддерева узлов в глубину, в порядке Родитель-Дети.
             */
            template <bool constant>
            class iterator {
            public:
                typedef typename std::conditional<
                    constant, const Node *, Node *>::type pointer;

                iterator(pointer root): current(root) {}
                
                iterator & operator++(int unused) { return this->operator++(); }
                iterator & operator++() {
                    if (!current) { return *this; }
                    if (current->children.empty()) {
                        moveToNextSibling();
                    } else {
                        current = current->children.at(0);
                        siblingIndices.push_back(0);
                    }
                    return *this;
                }
                pointer operator*() { return current; }
                size_t level() { return siblingIndices.size(); }

            private:
                void moveToNextSibling() {
                    pointer node = current;
                    size_t index;
                    do {
                        node = node->parent;
                        if (!node) { break; }
                        index = siblingIndices.back();
                        siblingIndices.pop_back();
                    } while (index >= node->children.size() - 1);
                    if (node) {
                        current = node->children.at(index + 1);
                        siblingIndices.push_back(index + 1);
                    } else {
                        current = nullptr;
                    }
                }

                pointer current;
                std::vector<size_t> siblingIndices;
            };

            /** Возвращает начальный mutable итератор обхода поддерева. */
            iterator<false> subtree_begin() { return iterator<false>(this); }
            /** Возвращает конечный mutable итератор обхода поддерева. */
            iterator<false> subtree_end() { return iterator<false>(nullptr); }
            /** Возвращает начальный const итератор обхода поддерева. */
            iterator<true> subtree_begin() const { return iterator<true>(this); }
            /** Возвращает конечный const итератор обхода поддерева. */
            iterator<true> subtree_end() const { return iterator<true>(nullptr); }

            /** Устанавливает тип узла. */
            void setKind(NodeKind kind) {
                if (this->kind == kind) { return; }
                this->kind = kind;
                owner.recomputeKey(this);
            }
            /** Возвращает тип узла. */
            NodeKind getKind() const { return kind; }

            /**
             * Собственный неизменяемый ключ узла.
             * Для модификации следует удалить узел и создать новый.
             * @see subtreeKey
             */
            const Key & ownKey() const { return nodeKey; }
            /**
             * Ключ поддерева с корнем в данном узле.
             * Включает в себя собственный ключ узла и ключи дочерних поддеревьев.
             * Обновляется при модификации поддерева (присоединении и отсоединении узлов).
             * @see ownKey
             */
            const Key & subtreeKey() const { return computedKey; }

            /** Устанавливает значение узла. */
            void setValue(const Value &newValue) { nodeValue = newValue; }
            /** Возвращает значение узла. */
            Value & getValue() { return nodeValue; }
            /** Возвращает значение узла. */
            const Value & getValue() const { return nodeValue; }

            /** Возвращает значение "у узла отсутствуют дочерние узелы?". */
            bool isLeaf() const { return children.empty(); }
            /** Возвращает количество дочерних узлов. */
            size_t childCount() const { return children.size(); }

            Node * child(size_t index) { return children.at(index); }
            const Node * child(size_t index) const { return children.at(index); }

            typename children_t::iterator begin() { return children.begin(); }
            typename children_t::iterator end()   { return children.end(); }
            typename children_t::const_iterator begin() const { return children.cbegin(); }
            typename children_t::const_iterator end()   const { return children.cend(); }
            typename children_t::const_iterator cbegin() const { return children.cbegin(); }
            typename children_t::const_iterator cend()   const { return children.cend(); }

            /**
             * Присоединяет узел child в качестве дочернего.
             * @param child присоединяемый узел; не должен уже находится в составе дерева
             * @see AndOrTree::attach(parent, child)
             */
            Node * attach(Node *child) {
                owner.attach(this, child);
                return this;
            }

            /**
             * Создаёт новый узел и присоединяет к данному узлу.
             * @see attach(child)
             * @see AndOrTree::create(kind, key, value)
             */
            Node * append(NodeKind kind, const Key &key, const Value &value) {
                return attach(owner.create(kind, key, value));
            }

            /**
             * Создаёт и возвращает поверхностную копию узла (без дочерних узлов),
             * владельцем которого будет то же дерево.
             * @see shallowClone(targetOwner)
             */
            Node * shallowClone() { return shallowClone(owner); }
            /**
             * Создаёт и возвращает поверхностную копию узла (без дочерних узлов),
             * владельцем которого становиться дерево targetOwner.
             * @see shallowClone()
             */
            Node * shallowClone(tree_t &targetOwner) const {
                return targetOwner.create(kind, nodeKey, nodeValue);
            }

            /**
             * Создаёт и возвращает глубокую копию узла (включая дочерние узлы),
             * владельцем которого будет то же дерево.
             * @see deepClone(targetOwner)
             */
            Node * deepClone() { return deepClone(owner); }
            /**
             * Создаёт и возвращает глубокую копию узла (включая дочерние узлы),
             * владельцем которого становиться дерево targetOwner.
             * @see deepClone()
             */
            Node * deepClone(tree_t &targetOwner) const {
                auto clonedParent = shallowClone(targetOwner);
                for (auto child : children) {
                    auto clonedChild = child->deepClone(targetOwner);
                    clonedChild->parent = clonedParent;
                    clonedParent->children.push_back(clonedChild);
                }
                clonedParent->computedKey = computedKey;
                return clonedParent;
            }

            /**
             * Отсоединяет узел от родительского узла.
             * Осоединение узла не означает его удаление: для удаления узла достаточно
             * вызвать delete над ним, что в том числе отсоеденит его.
             * @see AndOrTree::detach(node)
             */
            Node * detach() {
                return owner.detach(this);
            }

            /** Возвращает родительский узел. */
            const Node * const getParent() {
                return parent;
            }

            /** Отсоединяет и удаляет узел. */
            ~Node() {
                detach();
            }

		private:
            Node(tree_t &owner, Node *parent, NodeKind kind, const Key &key, const Value &value):
                owner(owner),
                parent(parent),
                kind(kind),
                nodeKey(key),
                computedKey(key),
                nodeValue(value)
            {}
			
            /* delete */ Node(const Node &) { assert(false); }
            /* delete */ Node & operator=(const Node &) { assert(false); }

            /** Владелец узла. */
            tree_t &owner;
            /** Родительский узел. */
			Node *parent;
            /** Тип узла. */
            NodeKind kind;
            /** Собственный ключ узла. */
            const Key nodeKey;
            /** Ключ поддерева с корнем в данном узле. */
            Key computedKey;
            Value nodeValue;
            /** Список дочерних узлов. Не следует модифицировать напрямую. */
            std::vector<Node *> children;
		};

        template <typename Key, typename Value, bool constant>
        bool operator==(
            const typename Node<Key, Value>::template iterator<constant> &it1,
            const typename Node<Key, Value>::template iterator<constant> &it2)
        {
            return it1.current == it2.current;
        }
        template <typename Key, typename Value, bool constant>
        bool operator!=(
            const typename Node<Key, Value>::template iterator<constant> &it1,
            const typename Node<Key, Value>::template iterator<constant> &it2)
        {
            return !(it1 == it2);
        }

        namespace design {
            /** Выводит в поток stream подряд элементы space в количестве size. */
            template <typename Stream, typename Space>
            Stream & indent(Stream &stream, const Space &space, size_t size) {
                for (size_t i = 0; i < size; i++) { stream << space; }
                return stream;
            }
            /**
             * Выводит в поток stream необходимое количество закрывающих
             * скобок closing с отступом space при возврате с уровня дерева
             * deepLevel на shallowLevel.
             * При deepLevel <= shallowLevel ничего не делает.
             */
            template <typename Stream, typename Space, typename Closing>
            Stream & closeLevels(
                Stream &stream,
                const Space &space,
                Closing closing,
                size_t deepLevel,
                size_t shallowLevel)
            {
                if (deepLevel <= shallowLevel) { return stream; }
                for (size_t i = deepLevel - 1; i >= shallowLevel && i-- != 0;) {
                    indent(stream, space, i) << closing;
                }
                return stream;
            }
            template <typename Stream, typename Key, typename Value>
            Stream & printNode(Stream &stream, const Node<Key, Value> &node, size_t level) {
                indent(stream, "  ", level) << node.ownKey();
                if (!node.isLeaf()) {
                    stream << " (" << node.subtreeKey() << ")";
                }
                return stream << ": "
                   << node.getValue() << " "
                   << node.getKind() << " "
                   << (node.isLeaf() ? "\n" : "[\n");
            }
        }

        template <typename Stream>
        Stream & operator<<(Stream &os, NodeKind kind) {
            switch (kind) {
                case NodeKind::AND:  return os << "&&";
                case NodeKind::OR:   return os << "||";
                case NodeKind::NONE: return os << "";
                default:             return os << "??";
            }
        }

        template <typename Stream, typename Key, typename Value>
        Stream & operator<<(Stream &os, const Node<Key, Value> &node) {
            size_t previousLevel = 0;
            for (auto it = node.subtree_begin(); *it; it++) {
                auto current = *it;
                size_t level = it.level();
                design::closeLevels(os, "  ", "]\n", previousLevel, level);
                design::printNode(os, *current, level);
                previousLevel = level;
            }
            design::closeLevels(os, "  ", "]\n", previousLevel, (size_t)0);
            return os;
        }
	}
}
