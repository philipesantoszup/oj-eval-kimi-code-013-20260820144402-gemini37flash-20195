/**
* implement a container like std::map
*/
#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

// only for std::less<T>
#include <functional>
#include <cstddef>
#include <utility>
#include <new>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template<
    class Key,
    class T,
    class Compare = std::less<Key>
> class map {
public:
    typedef pair<const Key, T> value_type;

private:
    enum Color { RED, BLACK };

    struct NodeBase {
        Color color;
        NodeBase *parent;
        NodeBase *left;
        NodeBase *right;

        NodeBase() : color(RED), parent(nullptr), left(nullptr), right(nullptr) {}
    };

    struct Node : public NodeBase {
        alignas(value_type) char storage[sizeof(value_type)];

        value_type *val_ptr() {
            return reinterpret_cast<value_type *>(storage);
        }
        const value_type *val_ptr() const {
            return reinterpret_cast<const value_type *>(storage);
        }

        template<typename... Args>
        void construct(Args&&... args) {
            new (storage) value_type(std::forward<Args>(args)...);
        }

        void destroy() {
            val_ptr()->~value_type();
        }
    };

    static Node* to_node(NodeBase *p) {
        return static_cast<Node *>(p);
    }
    static const Node* to_node(const NodeBase *p) {
        return static_cast<const Node *>(p);
    }

    NodeBase header;
    size_t total_size;
    Compare cmp;

    NodeBase *root() const {
        return header.parent;
    }
    void set_root(NodeBase *r) {
        header.parent = r;
        if (r) r->parent = &header;
    }
    NodeBase *most_left() const {
        return header.left;
    }
    void set_most_left(NodeBase *l) {
        header.left = l;
    }
    NodeBase *most_right() const {
        return header.right;
    }
    void set_most_right(NodeBase *r) {
        header.right = r;
    }

    static NodeBase *tree_min(NodeBase *x) {
        while (x->left != nullptr) {
            x = x->left;
        }
        return x;
    }

    static NodeBase *tree_max(NodeBase *x) {
        while (x->right != nullptr) {
            x = x->right;
        }
        return x;
    }

    static NodeBase *tree_increment(NodeBase *x) {
        if (x->right != nullptr) {
            x = x->right;
            while (x->left != nullptr) {
                x = x->left;
            }
            return x;
        } else {
            NodeBase *y = x->parent;
            while (y != nullptr && x == y->right) {
                x = y;
                y = y->parent;
            }
            if (x->right != y) {
                x = y;
            }
            return x;
        }
    }

    static NodeBase *tree_decrement(NodeBase *x) {
        if (x->color == RED && x->parent != nullptr && x->parent->parent == x) {
            // Header node (color == RED and parent points to root, whose parent points to header)
            return x->right;
        }
        if (x->left != nullptr) {
            NodeBase *y = x->left;
            while (y->right != nullptr) {
                y = y->right;
            }
            return y;
        } else {
            NodeBase *y = x->parent;
            while (y != nullptr && x == y->left) {
                x = y;
                y = y->parent;
            }
            return y;
        }
    }

    void rotate_left(NodeBase *x) {
        NodeBase *y = x->right;
        x->right = y->left;
        if (y->left != nullptr) {
            y->left->parent = x;
        }
        y->parent = x->parent;
        if (x == root()) {
            set_root(y);
        } else if (x == x->parent->left) {
            x->parent->left = y;
        } else {
            x->parent->right = y;
        }
        y->left = x;
        x->parent = y;
    }

    void rotate_right(NodeBase *x) {
        NodeBase *y = x->left;
        x->left = y->right;
        if (y->right != nullptr) {
            y->right->parent = x;
        }
        y->parent = x->parent;
        if (x == root()) {
            set_root(y);
        } else if (x == x->parent->right) {
            x->parent->right = y;
        } else {
            x->parent->left = y;
        }
        y->right = x;
        x->parent = y;
    }

    void rebalance_after_insert(NodeBase *x) {
        x->color = RED;
        while (x != root() && x->parent->color == RED) {
            if (x->parent == x->parent->parent->left) {
                NodeBase *y = x->parent->parent->right;
                if (y != nullptr && y->color == RED) {
                    x->parent->color = BLACK;
                    y->color = BLACK;
                    x->parent->parent->color = RED;
                    x = x->parent->parent;
                } else {
                    if (x == x->parent->right) {
                        x = x->parent;
                        rotate_left(x);
                    }
                    x->parent->color = BLACK;
                    x->parent->parent->color = RED;
                    rotate_right(x->parent->parent);
                }
            } else {
                NodeBase *y = x->parent->parent->left;
                if (y != nullptr && y->color == RED) {
                    x->parent->color = BLACK;
                    y->color = BLACK;
                    x->parent->parent->color = RED;
                    x = x->parent->parent;
                } else {
                    if (x == x->parent->left) {
                        x = x->parent;
                        rotate_right(x);
                    }
                    x->parent->color = BLACK;
                    x->parent->parent->color = RED;
                    rotate_left(x->parent->parent);
                }
            }
        }
        root()->color = BLACK;
    }

    NodeBase *rebalance_after_erase(NodeBase *z) {
        NodeBase *y = z;
        NodeBase *x = nullptr;
        NodeBase *x_parent = nullptr;

        if (y->left == nullptr) {
            x = y->right;
        } else if (y->right == nullptr) {
            x = y->left;
        } else {
            y = y->right;
            while (y->left != nullptr) {
                y = y->left;
            }
            x = y->right;
        }

        if (y != z) {
            // Relink y into z's place
            z->left->parent = y;
            y->left = z->left;
            if (y != z->right) {
                x_parent = y->parent;
                if (x) x->parent = y->parent;
                y->parent->left = x;
                y->right = z->right;
                z->right->parent = y;
            } else {
                x_parent = y;
            }
            if (root() == z) {
                set_root(y);
            } else if (z->parent->left == z) {
                z->parent->left = y;
            } else {
                z->parent->right = y;
            }
            y->parent = z->parent;
            std::swap(y->color, z->color);
            y = z; // y points to node to be deleted
        } else {
            x_parent = y->parent;
            if (x) x->parent = y->parent;
            if (root() == z) {
                set_root(x);
            } else if (z->parent->left == z) {
                z->parent->left = x;
            } else {
                z->parent->right = x;
            }
            if (most_left() == z) {
                if (z->right == nullptr) {
                    set_most_left(z->parent == &header ? &header : z->parent);
                } else {
                    set_most_left(tree_min(x));
                }
            }
            if (most_right() == z) {
                if (z->left == nullptr) {
                    set_most_right(z->parent == &header ? &header : z->parent);
                } else {
                    set_most_right(tree_max(x));
                }
            }
        }

        if (y->color == BLACK) {
            while (x != root() && (x == nullptr || x->color == BLACK)) {
                if (x == x_parent->left) {
                    NodeBase *w = x_parent->right;
                    if (w->color == RED) {
                        w->color = BLACK;
                        x_parent->color = RED;
                        rotate_left(x_parent);
                        w = x_parent->right;
                    }
                    if ((w->left == nullptr || w->left->color == BLACK) &&
                        (w->right == nullptr || w->right->color == BLACK)) {
                        w->color = RED;
                        x = x_parent;
                        x_parent = x_parent->parent;
                    } else {
                        if (w->right == nullptr || w->right->color == BLACK) {
                            if (w->left) w->left->color = BLACK;
                            w->color = RED;
                            rotate_right(w);
                            w = x_parent->right;
                        }
                        w->color = x_parent->color;
                        x_parent->color = BLACK;
                        if (w->right) w->right->color = BLACK;
                        rotate_left(x_parent);
                        break;
                    }
                } else {
                    NodeBase *w = x_parent->left;
                    if (w->color == RED) {
                        w->color = BLACK;
                        x_parent->color = RED;
                        rotate_right(x_parent);
                        w = x_parent->left;
                    }
                    if ((w->right == nullptr || w->right->color == BLACK) &&
                        (w->left == nullptr || w->left->color == BLACK)) {
                        w->color = RED;
                        x = x_parent;
                        x_parent = x_parent->parent;
                    } else {
                        if (w->left == nullptr || w->left->color == BLACK) {
                            if (w->right) w->right->color = BLACK;
                            w->color = RED;
                            rotate_left(w);
                            w = x_parent->left;
                        }
                        w->color = x_parent->color;
                        x_parent->color = BLACK;
                        if (w->left) w->left->color = BLACK;
                        rotate_right(x_parent);
                        break;
                    }
                }
            }
            if (x) x->color = BLACK;
        }
        return y;
    }

    void reset_header() {
        header.color = RED;
        header.parent = nullptr;
        header.left = &header;
        header.right = &header;
        total_size = 0;
    }

    NodeBase *copy_tree(const NodeBase *other_node, NodeBase *parent_node) {
        if (!other_node) return nullptr;
        Node *n = new Node();
        try {
            n->construct(*(to_node(other_node)->val_ptr()));
        } catch (...) {
            delete n;
            throw;
        }
        n->color = other_node->color;
        n->parent = parent_node;
        try {
            n->left = copy_tree(other_node->left, n);
            n->right = copy_tree(other_node->right, n);
        } catch (...) {
            destroy_tree(n);
            throw;
        }
        return n;
    }

    void destroy_tree(NodeBase *node) {
        if (!node) return;
        destroy_tree(node->left);
        destroy_tree(node->right);
        to_node(node)->destroy();
        delete to_node(node);
    }

    bool keys_equal(const Key &a, const Key &b) const {
        return !cmp(a, b) && !cmp(b, a);
    }

    NodeBase *find_node(const Key &key) const {
        NodeBase *curr = root();
        while (curr != nullptr) {
            const Key &curr_key = to_node(curr)->val_ptr()->first;
            if (cmp(key, curr_key)) {
                curr = curr->left;
            } else if (cmp(curr_key, key)) {
                curr = curr->right;
            } else {
                return curr;
            }
        }
        return const_cast<NodeBase *>(&header);
    }

public:
    class const_iterator;
    class iterator {
        friend class map;
        friend class const_iterator;
    private:
        NodeBase *node;
        const map *container;

    public:
        iterator() : node(nullptr), container(nullptr) {}

        iterator(NodeBase *n, const map *c) : node(n), container(c) {}

        iterator(const iterator &other) : node(other.node), container(other.container) {}

        iterator &operator=(const iterator &other) {
            if (this != &other) {
                node = other.node;
                container = other.container;
            }
            return *this;
        }

        iterator operator++(int) {
            if (!container || !node || node == &container->header) {
                throw invalid_iterator();
            }
            iterator temp = *this;
            node = tree_increment(node);
            return temp;
        }

        iterator &operator++() {
            if (!container || !node || node == &container->header) {
                throw invalid_iterator();
            }
            node = tree_increment(node);
            return *this;
        }

        iterator operator--(int) {
            if (!container || !node) {
                throw invalid_iterator();
            }
            if (node == container->most_left() || container->empty()) {
                throw invalid_iterator();
            }
            iterator temp = *this;
            node = tree_decrement(node);
            return temp;
        }

        iterator &operator--() {
            if (!container || !node) {
                throw invalid_iterator();
            }
            if (node == container->most_left() || container->empty()) {
                throw invalid_iterator();
            }
            node = tree_decrement(node);
            return *this;
        }

        value_type &operator*() const {
            if (!container || !node || node == &container->header) {
                throw invalid_iterator();
            }
            return *(to_node(node)->val_ptr());
        }

        value_type *operator->() const noexcept {
            return to_node(node)->val_ptr();
        }

        bool operator==(const iterator &rhs) const {
            return node == rhs.node && container == rhs.container;
        }

        bool operator==(const const_iterator &rhs) const {
            return node == rhs.node && container == rhs.container;
        }

        bool operator!=(const iterator &rhs) const {
            return !(*this == rhs);
        }

        bool operator!=(const const_iterator &rhs) const {
            return !(*this == rhs);
        }
    };

    class const_iterator {
        friend class map;
        friend class iterator;
    private:
        const NodeBase *node;
        const map *container;

    public:
        const_iterator() : node(nullptr), container(nullptr) {}

        const_iterator(const NodeBase *n, const map *c) : node(n), container(c) {}

        const_iterator(const const_iterator &other) : node(other.node), container(other.container) {}

        const_iterator(const iterator &other) : node(other.node), container(other.container) {}

        const_iterator &operator=(const const_iterator &other) {
            if (this != &other) {
                node = other.node;
                container = other.container;
            }
            return *this;
        }

        const_iterator operator++(int) {
            if (!container || !node || node == &container->header) {
                throw invalid_iterator();
            }
            const_iterator temp = *this;
            node = tree_increment(const_cast<NodeBase *>(node));
            return temp;
        }

        const_iterator &operator++() {
            if (!container || !node || node == &container->header) {
                throw invalid_iterator();
            }
            node = tree_increment(const_cast<NodeBase *>(node));
            return *this;
        }

        const_iterator operator--(int) {
            if (!container || !node) {
                throw invalid_iterator();
            }
            if (node == container->most_left() || container->empty()) {
                throw invalid_iterator();
            }
            const_iterator temp = *this;
            node = tree_decrement(const_cast<NodeBase *>(node));
            return temp;
        }

        const_iterator &operator--() {
            if (!container || !node) {
                throw invalid_iterator();
            }
            if (node == container->most_left() || container->empty()) {
                throw invalid_iterator();
            }
            node = tree_decrement(const_cast<NodeBase *>(node));
            return *this;
        }

        const value_type &operator*() const {
            if (!container || !node || node == &container->header) {
                throw invalid_iterator();
            }
            return *(to_node(node)->val_ptr());
        }

        const value_type *operator->() const noexcept {
            return to_node(node)->val_ptr();
        }

        bool operator==(const iterator &rhs) const {
            return node == rhs.node && container == rhs.container;
        }

        bool operator==(const const_iterator &rhs) const {
            return node == rhs.node && container == rhs.container;
        }

        bool operator!=(const iterator &rhs) const {
            return !(*this == rhs);
        }

        bool operator!=(const const_iterator &rhs) const {
            return !(*this == rhs);
        }
    };

    map() : total_size(0), cmp(Compare()) {
        reset_header();
    }

    map(const Compare &c) : total_size(0), cmp(c) {
        reset_header();
    }

    map(const map &other) : total_size(0), cmp(other.cmp) {
        reset_header();
        if (other.root() != nullptr) {
            NodeBase *r = copy_tree(other.root(), &header);
            set_root(r);
            set_most_left(tree_min(r));
            set_most_right(tree_max(r));
            total_size = other.total_size;
        }
    }

    map &operator=(const map &other) {
        if (this == &other) {
            return *this;
        }
        clear();
        cmp = other.cmp;
        if (other.root() != nullptr) {
            NodeBase *r = copy_tree(other.root(), &header);
            set_root(r);
            set_most_left(tree_min(r));
            set_most_right(tree_max(r));
            total_size = other.total_size;
        }
        return *this;
    }

    ~map() {
        clear();
    }

    T &at(const Key &key) {
        NodeBase *n = find_node(key);
        if (n == &header) {
            throw index_out_of_bound();
        }
        return to_node(n)->val_ptr()->second;
    }

    const T &at(const Key &key) const {
        NodeBase *n = find_node(key);
        if (n == &header) {
            throw index_out_of_bound();
        }
        return to_node(n)->val_ptr()->second;
    }

    T &operator[](const Key &key) {
        NodeBase *x = root();
        NodeBase *y = &header;
        bool comp = true;
        while (x != nullptr) {
            y = x;
            comp = cmp(key, to_node(x)->val_ptr()->first);
            if (comp) {
                x = x->left;
            } else if (cmp(to_node(x)->val_ptr()->first, key)) {
                x = x->right;
            } else {
                return to_node(x)->val_ptr()->second;
            }
        }

        Node *z = new Node();
        try {
            z->construct(key, T());
        } catch (...) {
            delete z;
            throw;
        }

        z->parent = y;
        z->left = nullptr;
        z->right = nullptr;

        if (y == &header) {
            set_root(z);
            set_most_left(z);
            set_most_right(z);
        } else if (comp) {
            y->left = z;
            if (y == most_left()) {
                set_most_left(z);
            }
        } else {
            y->right = z;
            if (y == most_right()) {
                set_most_right(z);
            }
        }

        rebalance_after_insert(z);
        total_size++;
        return z->val_ptr()->second;
    }

    const T &operator[](const Key &key) const {
        return at(key);
    }

    iterator begin() {
        return iterator(most_left(), this);
    }

    const_iterator cbegin() const {
        return const_iterator(most_left(), this);
    }

    iterator end() {
        return iterator(&header, this);
    }

    const_iterator cend() const {
        return const_iterator(&header, this);
    }

    bool empty() const {
        return total_size == 0;
    }

    size_t size() const {
        return total_size;
    }

    void clear() {
        destroy_tree(root());
        reset_header();
    }

    pair<iterator, bool> insert(const value_type &value) {
        NodeBase *x = root();
        NodeBase *y = &header;
        bool comp = true;
        while (x != nullptr) {
            y = x;
            comp = cmp(value.first, to_node(x)->val_ptr()->first);
            if (comp) {
                x = x->left;
            } else if (cmp(to_node(x)->val_ptr()->first, value.first)) {
                x = x->right;
            } else {
                return pair<iterator, bool>(iterator(x, this), false);
            }
        }

        Node *z = new Node();
        try {
            z->construct(value);
        } catch (...) {
            delete z;
            throw;
        }

        z->parent = y;
        z->left = nullptr;
        z->right = nullptr;

        if (y == &header) {
            set_root(z);
            set_most_left(z);
            set_most_right(z);
        } else if (comp) {
            y->left = z;
            if (y == most_left()) {
                set_most_left(z);
            }
        } else {
            y->right = z;
            if (y == most_right()) {
                set_most_right(z);
            }
        }

        rebalance_after_insert(z);
        total_size++;
        return pair<iterator, bool>(iterator(z, this), true);
    }

    void erase(iterator pos) {
        if (pos.container != this || pos.node == nullptr || pos.node == &header) {
            throw invalid_iterator();
        }
        NodeBase *y = rebalance_after_erase(pos.node);
        to_node(y)->destroy();
        delete to_node(y);
        total_size--;
    }

    size_t count(const Key &key) const {
        return find_node(key) != &header ? 1 : 0;
    }

    iterator find(const Key &key) {
        return iterator(find_node(key), this);
    }

    const_iterator find(const Key &key) const {
        return const_iterator(find_node(key), this);
    }
};

}

#endif
