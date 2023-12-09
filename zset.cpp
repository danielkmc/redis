#include <assert.h>
#include <string.h>
#include <stdlib.h>
// project
#include "zset.hpp"
#include "common.hpp"


static ZNode *znode_new(const char *name, size_t len, double score) {
    ZNode *node = (ZNode *)malloc(sizeof(ZNode) + len);
    assert(node); // TODO: change to check
    avl_init(&node->tree);
    node->hmap.next = NULL;
    node->hmap.hcode = str_hash((uint8_t *)name, len);
    node->score = score;
    node->len = len;
    memcpy(&node->name[0], name, len);
    return node;
}

static uint32_t min(size_t lhs, size_t rhs) {
    return lhs < rhs ? lhs : rhs;
}

static bool zless(
    AVLNode *lhs, double score, const char *name, size_t len)
{
    ZNode *zl = container_of(lhs, ZNode, tree);
    if (zl->score != score) {
        return zl->score < score;
    }
    int rv = memcmp(zl->name, name, min(zl->len, len));
    if (rv != 0) {
        return rv < 0;
    }
    return zl->len < len;
}

static bool zless(AVLNode *lhs, AVLNode *rhs) {
    ZNode *zr = container_of(rhs, ZNode, tree);
    return zless(lhs, zr->score, zr->name, zr->len);
}

static void tree_add(ZSet *zset, ZNode *node) {
    if (!zset->tree) {
        zset->tree = &node->tree;
        return;
    }

    AVLNode *cur = zset->tree;
    while (true) {
        AVLNode **from = zless(&node->tree, cur) ? &cur->left : &cur->right;
        if (!*from) {
            *from = &node->tree;
            node->tree.parent = cur;
            zset->tree = avl_fix(&node->tree);
            break;
        }
        cur = *from;
    }
}

static void zset_update(ZSet *zset, ZNode *node, double score) {
    if (node->score == score) {
        return;
    }
    zset->tree = avl_del(&node->tree);
    node->score = score;
    avl_init(&node->tree);
    tree_add(zset, node);
}

bool zset_add(ZSet *zset, const char *name, size_t len, double score) {
    ZNode *node = zset_lookup(zset, name, len);
    if (node) {
        zset_update(zset, node, score);
        return false;
    } else {
        node = znode_new(name, len, score);
        hm_insert(&zset->hmap, &node->hmap);
        tree_add(zset, node);
        return true;
    }
}

struct HKey {
    HNode node;
    const char *name = NULL;
    size_t len = 0;
};

static bool hcmp(HNode *node, HNode *key) {
    if (node->hcode != key->hcode) {
        return false;
    }
    ZNode *znode = container_of(node, ZNode, hmap);
    HKey *hkey = container_of(key, HKey, node);
    if (znode->len != hkey->len) {
        return false;
    }
    return 0 == memcmp(znode->name, hkey->name, znode->len);
}

ZNode *zset_lookup(ZSet *zset, const char *name, size_t len) {
    if (!zset->tree) {
        return NULL;
    }

    HKey key;
    key.node.hcode = str_hash((uint8_t *)name, len);
    key.name = name;
    key.len = len;
    HNode *found = hm_lookup(&zset->hmap, &key.node, &hcmp);
    if (!found) {
        return NULL;
    }

    return container_of(found, ZNode, hmap);
}

// Return node in ZSet that is associated with name `name`.
ZNode *zset_pop(ZSet *zset, const char *name, size_t len) {
    if (!zset->tree) {
        return NULL;
    }

    HKey key;
    key.node.hcode = str_hash((uint8_t *)name, len);
    key.name = name;
    key.len = len;
    HNode *found = hm_pop(&zset->hmap, &key.node, &hcmp);
    if (!found) {
        return NULL;
    }

    ZNode *node = container_of(found, ZNode, hmap);
    zset->tree = avl_del(&node->tree);
    return node;
}

// Returns the ZNode with AVLNode rank `offset` away from the node matching, or
// the next highest, (score, name) tuple. 
ZNode *zset_query(
    ZSet *zset, double score, const char *name, size_t len, int64_t offset)
{
    AVLNode *found = NULL;
    AVLNode *cur = zset->tree;
    while (cur) {
        if (zless(cur, score, name, len)) {
            cur = cur->right;
        } else {
            found = cur;
            cur = cur->left;
        }
    }

    if (found) {
        found = avl_offset(found, offset);
    }
    return found ? container_of(found, ZNode, tree) : NULL;
}

// Returns the rank of `node` in its AVL tree.
int64_t zset_zrank(ZSet *zset, const char *name, size_t len) {
    if (!zset) {
        return 0;
    }
    ZNode *znode = zset_lookup(zset, name, len);
    if (!znode) {
        return 0;
    }
    AVLNode *node = &znode->tree;
    if (!node) {
        return 0;
    }
    // find how many are ranked under
    int64_t rank = node->cnt;
    if (node->right) {
        rank -= node->right->cnt;
    }
    AVLNode *cur = node;
    AVLNode *parent = node->parent;
    // traverse up to the parent of the root
    while (parent) {
        if (cur == parent->right) {
            rank += parent->cnt - cur->cnt;
        }
        cur = parent;
        parent = parent->parent;
    }
    return rank; 
}

int64_t zset_zcount(ZSet *zset, int64_t low, int64_t high) {
    if (!zset) {
        return -1;
    }
    AVLNode *cur = zset->tree;
    int64_t lhs = 0;
    while (cur) {
        ZNode *znode = container_of(cur, ZNode, tree);
        if (znode->score < low) {
            lhs++;
            if (cur->left) {
                lhs += cur->left->cnt;
            }
            cur = cur->right;
        } else {
            cur = cur->left;
        }
    }
    int64_t rhs = 0;
    while (cur) {
        ZNode *znode = container_of(cur, ZNode, tree);
        if (znode->score < high) {
            rhs++;
            if (cur->left) {
                rhs += cur->left->cnt;
            }
            cur = cur->right;
        } else {
            cur = cur->left;
        }
    }
    return rhs - lhs;
}

void znode_del(ZNode *node) {
    free(node);
}

// Deletes entire subtree rooted at `node`.
static void tree_dispose(AVLNode *node) {
    if (!node) {
        return;
    }
    tree_dispose(node->left);
    tree_dispose(node->right);
    znode_del(container_of(node, ZNode, tree));
}

void zset_dispose(ZSet *zset) {
    tree_dispose(zset->tree);
    hm_destroy(&zset->hmap);
}