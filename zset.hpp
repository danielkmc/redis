#pragma once

#include "avl.hpp"
#include "hashtable.hpp"


struct ZSet {
    AVLNode *tree = NULL;
    HMap hmap;
};

struct ZNode {
    AVLNode tree;
    HNode hmap;
    double score = 0;
    size_t len = 0;
    char name[0];
};

bool zset_add(ZSet *zset, const char *name, size_t len, double score);
ZNode *zset_lookup(ZSet *zset, const char *name, size_t len);
ZNode *zset_pop(ZSet *zset, const char *name, size_t len);
ZNode *zset_query(
    ZSet *zset, double score, const char *name, size_t len, int64_t offset
);
int64_t zset_zrank(ZSet *zset, const char *name, size_t len);
int64_t zset_zcount(ZSet *zset, int64_t low, int64_t high);
void zset_dispose(ZSet *zset);
void znode_del(ZNode *node);