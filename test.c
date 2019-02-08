#include <assert.h>
#include <stdio.h>

#include "src/quadtree.h"

#define test(fn) \
        printf("\x1b[33m" # fn "\x1b[0m "); \
        test_##fn(); \
        puts("\x1b[1;32m ✓ \x1b[0m");

void
print_node(quadtree_node_t *node)
{
        printf("(%lf, %lf)\n", node->point->x, node->point->y);
}

void descent(quadtree_node_t *node){
        if(node->bounds != NULL)
                printf("{ nw.x:%f, nw.y:%f, se.x:%f, se.y:%f }: ", node->bounds->nw->x,
                                node->bounds->nw->y, node->bounds->se->x, node->bounds->se->y);
}

void ascent(quadtree_node_t *node){
        printf("\n");
}

static void
test_node(){
        quadtree_node_t *node = quadtree_node_new();
        assert(!quadtree_node_isleaf(node));
        assert(quadtree_node_isempty(node));
        assert(!quadtree_node_ispointer(node));
        free(node);
}

static void
test_bounds(){
        quadtree_bounds_t *bounds = quadtree_bounds_new();

        assert(bounds);
        assert(bounds->nw->x == INFINITY);
        assert(bounds->se->x == -INFINITY);

        quadtree_bounds_extend(bounds, 5.0, 5.0);
        assert(bounds->nw->x == 5.0);
        assert(bounds->se->x == 5.0);

        quadtree_bounds_extend(bounds, 10.0, 10.0);
        assert(bounds->nw->y == 10.0);
        assert(bounds->nw->y == 10.0);
        assert(bounds->se->y == 5.0);
        assert(bounds->se->y == 5.0);

        assert(bounds->width == 5.0);
        assert(bounds->height == 5.0);

        quadtree_bounds_free(bounds);
}

void
do_nothing(void *arg) {}


/* Convenience function */
static quadtree_node_t *
grab_first_node_from_query(quadtree_t *tree, double x, double y, double radius)
{
        quadtree_node_t *node;
        quadtree_node_list_t *query_result = quadtree_search_bounds(tree, x, y, radius);
        assert(query_result != NULL);

        /* Grab first result. Should be only one */
        node = query_result->node;
        quadtree_node_list_free(query_result);
        return node;
}

static void
test_tree_condense()
{
        int val = 10;
        quadtree_t *tree = quadtree_new(0, 0, 10, 10);

        /* Ensure tree has two leaves where I expect them. */
        assert(quadtree_insert(tree, 3, 8, &val) == 1);
        assert(quadtree_insert(tree, 2, 2, &val) == 1);
        assert(quadtree_node_ispointer(tree->root));
        assert(quadtree_node_isleaf(tree->root->nw));
        assert(quadtree_node_isempty(tree->root->ne));
        assert(quadtree_node_isleaf(tree->root->sw));
        assert(quadtree_node_isempty(tree->root->se));

        /* Ensure tree condensed into a leaf properly */
        quadtree_clear_leaf(tree->root->sw);
        assert(quadtree_node_isleaf(tree->root));
        assert(tree->root->nw == NULL);
        assert(tree->root->ne == NULL);
        assert(tree->root->sw == NULL);
        assert(tree->root->se == NULL);

        /* Ensure root condensed into an empty properly */
        quadtree_clear_leaf(tree->root);
        assert(quadtree_node_isempty(tree->root));
}

static void
test_tree(){
        int val = 10;

        quadtree_t *tree = quadtree_new(0, 0, 10, 10);
        quadtree_node_t *node;
        quadtree_node_list_t *query_result;

        /* Test that quads can be reused. */
        assert(quadtree_insert(tree, 4, 4, &val) == 1);
        node = grab_first_node_from_query(tree, 2.5, 2.5, 2);
        assert (quadtree_clear_leaf(node) != NULL);
        /* New val for later sanity testing */
        int val2 = 42;
        assert(quadtree_insert(tree, 3, 4, &val2) == 1);

        /* Test that quads can be moved.
         * -- Lateral test from one quadrant to another under the same parent */

        /* Grab old node */
        node = grab_first_node_from_query(tree, 3, 4 ,1);
        assert(node->point->x == 3 && node->point->y == 4);
        /* Move from SW to NW */
        assert(quadtree_move_leaf(tree, node, quadtree_point_new(3, 8)) == 1);

        node = grab_first_node_from_query(tree, 3, 8, 2);
        /* Check that query grabbed correct new node */
        assert(node->point->x == 3 && node->point->y == 8);
        assert(*(int *)node->key == 42);

        /* Check that old node is gone */
        query_result = quadtree_search_bounds(tree, 3, 4, 2);
        assert(query_result == NULL);
        quadtree_node_list_free(query_result);

        assert(quadtree_insert(tree, 1, 1, &val) == 1);
        assert(quadtree_insert(tree, 6, 0, &val) == 1);
        assert(quadtree_insert(tree, 6, 6, &val) == 1);
        assert(quadtree_insert(tree, 3.1, 4, &val) == 1);
        assert(quadtree_insert(tree, 3, 3, &val) == 1);
        assert(quadtree_insert(tree, 2, 1.2, &val) == 1);
        assert(quadtree_insert(tree, 2, 1, &val) == 1);
        assert(quadtree_insert(tree, 5, 1, &val) == 1);
        assert(quadtree_insert(tree, 1, 1.1, &val) == 1);
        assert(quadtree_insert(tree, 1, 2.1, &val) == 1);
        assert(quadtree_insert(tree, 1, 3.1, &val) == 1);
#if 0
        assert(quadtree_insert(tree, 0, 0, &val) == 0);
        assert(quadtree_insert(tree, 110.0, 110.0, &val) == 0);

        assert(quadtree_insert(tree, 8.0, 2.0, &val) != 0);
        assert(tree->length == 1);
        assert(tree->root->point->x == 8.0);
        assert(tree->root->point->y == 2.0);

        assert(quadtree_insert(tree, 0.0, 1.0, &val) == 0); /* failed insertion */
        assert(quadtree_insert(tree, 2.0, 3.0, &val) == 1); /* normal insertion */
        assert(quadtree_insert(tree, 2.0, 3.0, &val) == 2); /* replacement insertion */
        assert(tree->length == 2);
        assert(tree->root->point == NULL);

        assert(quadtree_insert(tree, 3.0, 1.1, &val) == 1);
        assert(tree->length == 3);
        assert(quadtree_search(tree, 3.0, 1.1)->x == 3.0);
#endif
        quadtree_walk(tree->root, ascent, descent);

        query_result = quadtree_search_bounds(tree, 2.5, 2.5, 5);
        quadtree_node_list_t *curr = query_result;
        printf("\nQuery results: ");
        while(curr != NULL && curr->node != NULL) {
                printf("(%lf, %lf) --> ", curr->node->point->x, curr->node->point->y);
                curr = curr->next;
        }
        printf("NULL\n");
        quadtree_node_list_free(query_result);

        query_result = quadtree_search_bounds(tree, 3, 8, 0.1);
        curr = query_result;
        printf("\nQuery results: ");
        while(curr != NULL && curr->node != NULL) {
                printf("(%lf, %lf) --> ", curr->node->point->x, curr->node->point->y);
                curr = curr->next;
        }
        printf("NULL\n");
        quadtree_node_list_free(query_result);

        quadtree_free(tree);
}

static void
test_points(){
        quadtree_point_t *point = quadtree_point_new(5, 6);
        assert(point->x == 5);
        assert(point->y == 6);
        quadtree_point_free(point);
}

int
main(int argc, const char *argv[]){
        printf("\nquadtree_t: %ld\n", sizeof(quadtree_t));
        printf("quadtree_node_t: %ld\n", sizeof(quadtree_node_t));
        printf("quadtree_bounds_t: %ld\n", sizeof(quadtree_bounds_t));
        printf("quadtree_point_t: %ld\n", sizeof(quadtree_point_t));
        test(tree);
        test(node);
        test(bounds);
        test(points);
        test(tree_condense);
}
