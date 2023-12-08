#include <assert.h>
#include <stdlib.h>
#include "hashtable.hpp"


// Initializes hashtable to `n`, must be power of 2. Mask takes the log2(n) - 1 
// significant bits to determine the position of a node. 
static void h_init(HTab *htab, size_t n) {
    assert(n > 0 && ((n - 1) & n) == 0);
    htab->tab = (HNode **)calloc(sizeof(HNode *), n);
    htab->mask = n - 1;
    htab->size = 0;
}

// Inserts an HNode `*node` into the hashtable `*htab`. Determines the position
// using the hashcode associated with the node modded by the mask. Insertion
// of the node places it at the beginning of the chain of nodes at its position.
static void h_insert(HTab *htab, HNode *node) {
    size_t pos = node->hcode & htab->mask;
    HNode *next = htab->tab[pos];
    node->next = next;
    htab->tab[pos] = node;
    htab->size++;
}

// hashtable look up subroutine.
static HNode **h_lookup(
    HTab *htab, HNode *key, bool (*cmp)(HNode *, HNode *))
{
    if (!htab->tab) {
        return NULL;
    }

    size_t pos = key->hcode & htab->mask;
    HNode **from = &htab->tab[pos];
    while (*from) {
        if (cmp(*from, key)) {
            return from;
        }
        from = &(*from)->next;
    }
    return NULL;
}

// Returns the HNode * `node` from the chain location `**from`. Places the next
// node in the chain in the place of the removed node. 
static HNode *h_detach(HTab *htab, HNode **from) {
    HNode *node = *from;
    *from = (*from)->next;
    htab->size--;
    return node;
}

const size_t k_resizing_work = 128;

static void hm_help_resizing(HMap *hmap) {
    if (hmap->ht2.tab == NULL) {
        return;
    }

    size_t nwork = 0;
    while (nwork < k_resizing_work && hmap->ht2.size > 0) {
        // scan for nodes from ht2 and move them to ht1
        HNode **from = &hmap->ht2.tab[hmap->resizing_pos];
        if (!*from) {
            hmap->resizing_pos++;
            continue;
        }

        h_insert(&hmap->ht1, h_detach(&hmap->ht2, from));
        nwork++;
    }

    if (hmap->ht2.size == 0) {
        // done
        free(hmap->ht2.tab);
        hmap->ht2 = HTab{};
    }
}

static void hm_start_resizing(HMap *hmap, bool upsize) {
    assert(hmap->ht2.tab == NULL);
    // change the hmap size to be bigger or smaller based on upsize flag
    hmap->ht2 = hmap->ht1;
    if (upsize) {
        h_init(&hmap->ht1, (hmap->ht1.mask + 1) * 2);
    } else {
        h_init(&hmap->ht1, (hmap->ht1.mask + 1) / 2);
    }
    hmap->resizing_pos = 0;
}

HNode *hm_lookup(
    HMap *hmap, HNode *key, bool (*cmp)(HNode *, HNode *))
{
    hm_help_resizing(hmap);
    HNode **from = h_lookup(&hmap->ht1, key, cmp);
    if (!from) {
        from = h_lookup(&hmap->ht2, key, cmp);
    }
    return from ? *from : NULL;
}

const size_t k_max_load_factor = 8;
const size_t k_min_load_factor = 4;

void hm_insert(HMap *hmap, HNode *node) {
    if (!hmap->ht1.tab) {
        h_init(&hmap->ht1, 4);
    }
    h_insert(&hmap->ht1, node);

    if (!hmap->ht2.tab) {
        // check whether we need to resize
        size_t load_factor = hmap->ht1.size / (hmap->ht1.mask + 1);
        if ((hmap->ht1.mask + 1) > 4 && load_factor <= k_min_load_factor) {
            hm_start_resizing(hmap, false);
        } else if (load_factor >= k_max_load_factor) {
            hm_start_resizing(hmap, true);
        }
    }
    hm_help_resizing(hmap);
}

HNode *hm_pop(
    HMap *hmap, HNode *key, bool (*cmp)(HNode *, HNode *))
{
    hm_help_resizing(hmap);
    HNode **from = h_lookup(&hmap->ht1, key, cmp);
    if (from) {
        return h_detach(&hmap->ht1, from);
    }
    from = h_lookup(&hmap->ht2, key, cmp);
    if (from) {
        return h_detach(&hmap->ht2, from);
    }
    return NULL;
}

size_t hm_size(HMap *hmap) {
    return hmap->ht1.size + hmap->ht2.size;
}

void hm_destroy(HMap *hmap) {
    free(hmap->ht1.tab);
    free(hmap->ht2.tab);
    *hmap = HMap{};
}