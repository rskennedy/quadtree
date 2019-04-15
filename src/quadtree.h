#ifndef __QUADTREE_H__
#define __QUADTREE_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _FAKE_MALLOC
#include "fake_malloc.h"

#else
//#include <rte_malloc.h>
#endif

#define QUADTREE_VERSION "0.0.1"

#include <stdlib.h>
#include <math.h>

typedef enum coordinate {
        NW,
        NE,
        SW,
        SE,
        NO_COORDINATE,
} coordinate_t;

typedef struct quadtree_point {
        double x;
        double y;
} quadtree_point_t;

typedef struct quadtree_bounds {
        quadtree_point_t *nw;
        quadtree_point_t *se;
        double width;
        double height;
} quadtree_bounds_t;

typedef struct quadtree_node {
        coordinate_t coord;
        unsigned int children_cnt;
        unsigned int weight;
        struct quadtree_node *parent;
        struct quadtree_node *ne;
        struct quadtree_node *nw;
        struct quadtree_node *se;
        struct quadtree_node *sw;
        quadtree_bounds_t *bounds;
        quadtree_point_t  *point;
        void *key;
} quadtree_node_t;

typedef struct quadtree_node_list {
        quadtree_node_t *node;
        struct quadtree_node_list *next;
} quadtree_node_list_t;

typedef struct quadtree {
        quadtree_node_t *root;
        void (*key_free)(void *key);
        unsigned int length;
} quadtree_t;


quadtree_point_t*
quadtree_point_new(double x, double y);

void
quadtree_point_free(quadtree_point_t *point);


quadtree_bounds_t*
quadtree_bounds_new();

quadtree_bounds_t*
quadtree_bounds_new_with_points(double xmin, double ymin, double xmax, double ymax);

void
quadtree_bounds_extend(quadtree_bounds_t *bounds, double x, double y);

void
quadtree_bounds_free(quadtree_bounds_t *bounds);


quadtree_node_t*
quadtree_node_new();

void
quadtree_node_free(quadtree_node_t *node, void (*value_free)(void*));

int
quadtree_node_ispointer(quadtree_node_t *node);

int
quadtree_node_isempty(quadtree_node_t *node);

int
quadtree_node_isleaf(quadtree_node_t *node);

void
quadtree_node_reset(quadtree_node_t* node, void (*key_free)(void*));

void
quadtree_node_unlink(quadtree_node_t *node);

void *
quadtree_clear_leaf(quadtree_node_t *node);

void *
quadtree_clear_leaf_with_condense(quadtree_t *tree,quadtree_node_t *node);

int
quadtree_move_leaf(quadtree_t *tree, quadtree_node_t **node, quadtree_point_t *point);

quadtree_node_t*
quadtree_node_with_bounds(double minx, double miny, double maxx, double maxy);

quadtree_node_list_t*
quadtree_node_list_new(quadtree_node_t *node);

void
quadtree_node_list_free(quadtree_node_list_t *list);

void
quadtree_node_list_add(quadtree_node_list_t **list_p, quadtree_node_t *node);

quadtree_t*
quadtree_new(double minx, double miny, double maxx, double maxy);

void
quadtree_free(quadtree_t *tree);

quadtree_point_t*
quadtree_search(quadtree_t *tree, double x, double y);

quadtree_node_t*
quadtree_node_search(quadtree_t *tree, double x, double y);

quadtree_node_list_t*
quadtree_search_bounds(quadtree_t *tree, double x, double y, double radius);


quadtree_node_list_t*
quadtree_search_bounds_include_partial(quadtree_t *tree, double x, double y, double radius);

int
quadtree_insert(quadtree_t *tree, double x, double y, void *key, quadtree_node_t **node_p);

void
quadtree_walk(quadtree_node_t *root,
              void (*descent)(quadtree_node_t *node),
              void (*ascent)(quadtree_node_t *node));


quadtree_node_t *
quadtree_find_optimal_split_quad(quadtree_t *tree);

#ifdef __cplusplus
}
#endif

#endif
