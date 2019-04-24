#include "quadtree.h"

/* helpers */

int
quadtree_node_ispointer(quadtree_node_t* node) {
        return node->nw != NULL;
        /*
        return node->nw != NULL
                && node->ne != NULL
                && node->sw != NULL
                && node->se != NULL
                && !quadtree_node_isleaf(node);
                */
}

int
quadtree_node_isempty(quadtree_node_t* node) {
        return node->point == NULL && node->nw == NULL;
        /*
        return node->children_cnt == 0 && node->point == NULL;
        int ret;
        ret = node->nw == NULL
                && node->ne == NULL
                && node->sw == NULL
                && node->se == NULL
                && !quadtree_node_isleaf(node);
        if (ret == 1 && node->children_cnt)
                printf("NUM CHILDREN %d\n", node->children_cnt);
        return ret;
        */
}

int
quadtree_node_isleaf(quadtree_node_t* node) {
        return node->point != NULL;
}

void
quadtree_node_reset(quadtree_node_t* node, void (*key_free)(void*)) {
        quadtree_point_free(node->point);
        (*key_free)(node->key);
}

/* api */
quadtree_node_t*
quadtree_node_new() {
        quadtree_node_t* node = malloc(sizeof(quadtree_node_t));
        // quadtree_node_t *node = rte_malloc("node", sizeof(quadtree_node_t), 0);
        if (node == NULL) {
                return NULL;
        }
        node->coord = NO_COORDINATE;
        node->parent = NULL;
        node->ne = NULL;
        node->nw = NULL;
        node->se = NULL;
        node->sw = NULL;
        node->point = NULL;
        node->bounds = NULL;
        node->key = NULL;
        node->children_cnt = 0;
        node->weight = 0;
        return node;
}

quadtree_node_t*
quadtree_node_with_bounds(double minx, double miny, double maxx, double maxy) {
        quadtree_node_t* node;
        if (!(node = quadtree_node_new()))
                return NULL;
        /*
        if(!(node->bounds = quadtree_bounds_new())) return NULL;
        quadtree_bounds_extend(node->bounds, maxx, maxy);
        quadtree_bounds_extend(node->bounds, minx, miny);
        */
        quadtree_bounds_t* bounds = quadtree_bounds_new_with_points(minx, miny, maxx, maxy);
        if (bounds == NULL)
                return NULL;
        node->bounds = bounds;
        return node;
}

void
quadtree_node_free(quadtree_node_t* node, void (*key_free)(void*)) {
        if (node->nw != NULL)
                quadtree_node_free(node->nw, key_free);
        if (node->ne != NULL)
                quadtree_node_free(node->ne, key_free);
        if (node->sw != NULL)
                quadtree_node_free(node->sw, key_free);
        if (node->se != NULL)
                quadtree_node_free(node->se, key_free);

        quadtree_bounds_free(node->bounds);
        quadtree_node_reset(node, key_free);
        // rte_free(node);
        free(node);
}
