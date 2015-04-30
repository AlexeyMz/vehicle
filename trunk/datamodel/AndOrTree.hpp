#pragma once

#include <assert.h>
#include <functional>
#include <iostream>

#include "Node.hpp"

namespace vehicle {
    namespace core {
        /**
         * И-ИЛИ дерево.
         */
        template <typename Key, typename Value>
        class AndOrTree /* final */ {
        public:
            typedef Key key_t;
			typedef Value value_t;
            typedef Node<Key, Value> node_t;
            
            friend class node_t;

            /**
             * Инициализирует новый экземпляр дерева.
             * @param computeKey Функция расчёта ключа поддерева; по-умолчанию
             *     возвращает минимальную возможную стоимость решения. 
             */
            AndOrTree(std::function<Key (node_t &)> computeKey = &(defaultComputeKey<Key, Value>)):
                computeKey(computeKey), root(nullptr) {}

            AndOrTree(const AndOrTree &source):
                computeKey(source.computeKey)
            {
                cloneFrom(source);
            }

            AndOrTree & operator=(const AndOrTree &source) {
                if (&source == this) { return; }
                this->~AndOrTree();
                computeKey = source.computeKey;
                cloneFrom(source);
            }

            ~AndOrTree() {
                if (root) { delete root; }
            }

            /**
             * Заменяет корень дерева на новый, возвращая предыдущий корень.
             * Не освобождает ресурсы предыдущего корня дерева.
             */
            node_t * replaceRoot(node_t *root) {
                assert(!root || (&root->owner == this && !root->parent));
                auto oldRoot = this->root;
                this->root = root;
                return oldRoot;
            }

            /**
             * Заменяет корень дерева на новый, освобождая ресурсы предыдущего корня.
             */
            void setRoot(node_t *root) {
                auto oldRoot = replaceRoot(root);
                delete oldRoot;
            }
            /** Возвращает текущий корень дерева. */
            node_t * getRoot() const {
                return root;
            }

            /**
             * Создаёт и возвращает новый узел, который в дальнейшем можно
             * присоединять к дереву или устанавливать в качестве корневого.
             * @param kind тип узла (и/или/другой)
             * @param key неизменяемый ключ узла
             * @param value изменяемое значение узла
             */
            node_t * create(NodeKind kind, const Key &key, const Value &value) {
                return new node_t(*this, nullptr, kind, key, value);
            }

            /** @see Node::attach(child) */
            void attach(node_t *parent, node_t *child) {
                assert(parent && &parent->owner == this);
                assert(child && &child->owner == this);
                assert(!child->parent && child != root);
                child->parent = parent;
                parent->children.push_back(child);
                recomputeKey(parent);
            }

            /** @see Node::detach() */
            node_t * detach(node_t *node) {
                assert(node && (node->parent || node == root));
                if (node == root) {
                    root = nullptr;
                } else {
                    auto parent = node->parent;
                    auto &children = parent->children;
                    auto it = std::find(std::begin(children), std::end(children), node);
                    assert(it != std::end(children));
                    children.erase(it);
                    node->parent = nullptr;
                    recomputeKey(parent);
                }
                return node;
            }

            void recomputeKey(node_t *node) {
                assert(node);
                node->computedKey = computeKey(*node);
                if (node->parent) { recomputeKey(node->parent); }
            }

        private:
            void cloneFrom(const AndOrTree &source) {
                if (source.root) {
                    root = source.root->deepClone(*this);
                }
            }

            /** Корень дерева. */
            node_t *root;
            /**
             * Функция расчёта ключа поддерева.
             * @see recomputeKey(node)
             * @see Node::subtreeKey()
             */
            std::function<Key (node_t &)> computeKey;
        };

        template <typename Key, typename Value>
        Key defaultComputeKey(Node<Key, Value> &node) {
            if (node.isLeaf()) {
                return node.ownKey();
            } else if (node.getKind() == NodeKind::AND) {
                Key key = node.ownKey();
                for (auto child : node) {
                    key = key + child->subtreeKey();
                }
                return key;
            } else if (node.getKind() == NodeKind::OR) {
                Key key = node.child(0)->subtreeKey();
                for (size_t i = 1; i < node.childCount(); i++) {
                    const Key &childKey = node.child(i)->subtreeKey();
                    if (childKey < key) { key = childKey; }
                }
                return node.ownKey() + key;
            } else {
                assert("Unknown non-leaf node kind" && false);
                return Key();
            }
        }

        template <typename Key, typename Value>
        std::ostream & operator<<(std::ostream &os, const AndOrTree<Key, Value> &tree) {
            if (tree.getRoot()) {
                os << *tree.getRoot();
            } else {
                os << "<empty>";
            }
            return os;
        }
    }
}
