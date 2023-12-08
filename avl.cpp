#include "avl.hpp"


// Returns the depth of the node. The root of the tree starts at depth 1.
static uint32_t avl_depth(AVLNode *node) {
    return node ? node->depth : 0;
}

// Returns the number of nodes in this subtree including the root.
static uint32_t avl_cnt(AVLNode *node) {
    return node ? node->cnt : 0;
}

// Returns the larger value between the two `lhs` and `rhs`.
static uint32_t max(uint32_t lhs, uint32_t rhs) {
    return lhs < rhs ? rhs : lhs;
}

// Updates the depth and cnt values for this node and its descendents.
static void avl_update(AVLNode *node) {
    node->depth = 1 + max(avl_depth(node->left), avl_depth(node->right));
    node->cnt = 1 + avl_cnt(node->left) + avl_cnt(node->right);
}

// Returns pointer `new_node` to node that took the place of `node` passed in.
// Rotates the imbalance from right to left. The right child of `node` becomes
// the parent of `node` and the left child of `new_node` becomes the right
// child of `node`. 
static AVLNode *rot_left(AVLNode *node) {
    AVLNode *new_node = node->right;
    if (new_node->left) {
        new_node->left->parent = node;
    }
    node->right = new_node->left;
    new_node->left = node;
    new_node->parent = node->parent;
    node->parent = new_node;
    avl_update(node);
    avl_update(new_node);
    return new_node;
}

// Returns pointer `new_node` to node that took the place of `node` passed in.
// Rotates the imbalance from left to right. The left child of `node` becomes
// the parent of `node` and the right child of `new_node` becomes the left
// child of `node`. 
static AVLNode *rot_right(AVLNode *node) {
    AVLNode *new_node = node->left;
    if (new_node->right) {
        new_node->right->parent = node;
    }
    node->left = new_node->right;
    new_node->right = node;
    new_node->parent = node->parent;
    node->parent = new_node;
    avl_update(node);
    avl_update(new_node);
    return new_node;
}

// Returns the new root node of the subtree defined by `root` after performing
// a left rotation on the left subtree (if needed) and a right rotation on this
// subtree.
static AVLNode *avl_fix_left(AVLNode *root) {
    if (avl_depth(root->left->left) < avl_depth(root->left->right)) {
        root->left = rot_left(root->left);
    }
    return rot_right(root);
}

// Returns the new root node of the subtree defined by `root` after performing
// a right rotation on the right subtree (if needed) and a left rotation on this
// subtree.
static AVLNode *avl_fix_right(AVLNode *root) {
    if (avl_depth(root->right->right) < avl_depth(root->right->left)) {
        root->right = rot_right(root->right);
    }
    return rot_left(root);
}

// Returns the root node of the entire AVL tree after fixing the balance of all
// ancestor nodes of `node`, and depth and counts of all nodes. 
AVLNode *avl_fix(AVLNode *node) {
    while (true) {
        avl_update(node);
        uint32_t l = avl_depth(node->left);
        uint32_t r = avl_depth(node->right);
        AVLNode **from = NULL;
        if (node->parent) {
            from = (node->parent->left == node) 
                ? &node->parent->left : &node->parent->right;
        }
        if (l == r + 2) {
            node = avl_fix_left(node);
        } else if (l + 2 == r) {
            node = avl_fix_right(node);
        }
        if (!from) {
            return node;
        }
        *from = node;
        node = node->parent;
    }
}

// Returns the root of the entire AVL tree `node` resided in. 
// If there's no right child, this function replaces `node` with the left
// subtree and calls `avl_fix` to fix the balance of the tree. If there is a
// right child, we find the next inorder node to replace `node` after fixing
// the balance of the tree. 
AVLNode *avl_del(AVLNode *node) {
    if (node->right == NULL) {
        AVLNode *parent = node->parent;
        if (node->left) {
            node->left->parent = parent;
        }
        if (parent) {
            (parent->left == node ? parent->left : parent->right) = node->left;
            return avl_fix(parent);
        } else {
            return node->left;
        }
    } else {
        AVLNode *victim = node->right;
        while (victim->left) {
            victim = victim->left;
        }
        AVLNode *root = avl_del(victim);

        // The victim replaces its values with node's parameters
        *victim = *node;
        if (victim->left) {
            victim->left->parent = victim;
        }
        if (victim->right) {
            victim->right->parent = victim;
        }
        AVLNode *parent = node->parent;
        if (parent) {
            (parent->left == node ? parent->left : parent->right) = victim;
            return root;
        } else {
            return victim;
        }
    }
}

// Returns the node that is `offset` away from `node` in AVL tree.
AVLNode *avl_offset(AVLNode *node, int64_t offset) {
    int64_t pos = 0;
    while (offset != pos) {
        if (pos < offset && pos + avl_cnt(node->right) >= offset) {
            node = node->right;
            pos += avl_cnt(node->left) + 1;
        } else if (pos > offset && pos - avl_cnt(node->left) <= offset) {
            node = node->left;
            pos -= avl_cnt(node->right) + 1;
        } else {
            AVLNode *parent = node->parent;
            if (!parent) {
                return NULL;
            }
            if (parent->right == node) {
                pos -= avl_cnt(node->left) + 1;
            } else {
                pos += avl_cnt(node->right) + 1;
            }
            node = parent;
        }
    }
    return node;
}