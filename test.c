#include <assert.h>
#include <stdio.h>
#include <time.h>

#include "src/quadtree.h"

#define test(fn) \
        printf("\x1b[33m" # fn "\x1b[0m "); \
        test_##fn(); \
        puts("\x1b[1;32m ✓ \x1b[0m");

#define QUOTE(x) #x
#define STR(x) QUOTE(x)

#define ASSERT_VERBOSE(cond, tree)                                    \
        do {                                                          \
                if (!(cond)) {                                        \
                        printf("Failed test: %s.                      \n", STR(cond)); \
                        printf("Walking tree...                       \n");            \
                        quadtree_walk((tree)->root, ascent, descent); \
                        exit(EXIT_FAILURE);                           \ 
                }                                                     \
        } while(0);                                                   \

        void
print_node(quadtree_node_t *node)
{
        printf("(%lf, %lf)                                   \n", node->point->x, node->point->y);
}

void descent(quadtree_node_t *node){
        if(node->bounds != NULL)
                printf("{ nw.x:%f, nw.y:%f, se.x:%f, se.y:%f }: ", node->bounds->nw->x,
                                node->bounds->nw->y, node->bounds->se->x, node->bounds->se->y);
        if (node->point != NULL)
                printf("%f:%f\n", node->point->x, node->point->y);
}

void ascent(quadtree_node_t *node){
        printf("\n");
}

void descent_draw(quadtree_node_t *node){
        if(node->bounds != NULL)
                printf("%f %f %f %f - ", node->bounds->nw->x,
                                node->bounds->nw->y, node->bounds->se->x, node->bounds->se->y);
        if (node->point != NULL)
                printf("%f:%f\n", node->point->x, node->point->y);

}

void ascent_draw(quadtree_node_t *node){
        printf("\n");
}

static void
test_node(){
        quadtree_node_t *node = quadtree_node_new();
        assert(!quadtree_node_isleaf(node));
        assert(quadtree_node_isempty(node));
        assert(!quadtree_node_ispointer(node));
        rte_free(node);
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
        assert(quadtree_insert(tree, 3, 8, &val, NULL) == 1);
        printf("\n1 child---\n");
        quadtree_walk(tree->root, ascent_draw, descent_draw);

        assert(quadtree_insert(tree, 2, 2, &val, NULL) == 1);
        assert(quadtree_node_ispointer(tree->root));
        assert(quadtree_node_isleaf(tree->root->nw));
        assert(quadtree_node_isempty(tree->root->ne));
        assert(quadtree_node_isleaf(tree->root->sw));
        assert(quadtree_node_isempty(tree->root->se));

        printf("\nBEFORE---\n");
        quadtree_walk(tree->root, ascent_draw, descent_draw);

        /* Ensure tree condensed into a leaf properly */
        quadtree_clear_leaf_with_condense(tree, quadtree_node_search(tree, 2.0, 2.0));
        //quadtree_clear_leaf_with_condense(tree, tree->root->sw);
        printf("\nAFTER---\n");
        quadtree_walk(tree->root, ascent_draw, descent_draw);

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
test_leaf_move(void)
{
        int val = 10;
        quadtree_t *tree = quadtree_new(0, 0, 10, 10);

        /* Ensure tree has two leaves where I expect them. */
        assert(quadtree_insert(tree, 3, 8, &val, NULL) == 1);
        assert(quadtree_insert(tree, 2, 2, &val, NULL) == 1);
        assert(quadtree_node_ispointer(tree->root));
        assert(quadtree_node_isleaf(tree->root->nw));
        assert(quadtree_node_isempty(tree->root->ne));
        assert(quadtree_node_isleaf(tree->root->sw));
        assert(quadtree_node_isempty(tree->root->se));

        printf("\nBEFORE---\n");
        quadtree_walk(tree->root, ascent_draw, descent_draw);

        /* Ensure tree condensed into a leaf properly */
        quadtree_point_t *point = malloc(sizeof(quadtree_point_t));
        if (point == NULL) {
                return;
        }
        point->x = 8;
        point->y = 8;
        quadtree_node_t *node_the_node = quadtree_node_search(tree, 3.0, 8.0);
        assert(quadtree_move_leaf(tree, &node_the_node, point) == 1);

        printf("\nAFTER---\n");
        quadtree_walk(tree->root, ascent_draw, descent_draw);
}

static void
test_leaf_move_complex(void)
{
        int val = 10;
        int i;
        quadtree_t *tree = quadtree_new(0, 0, 10, 10);

        /* Ensure tree has two leaves where I expect them. */
        assert(quadtree_insert(tree, 3, 8, &val, NULL) == 1);
        //assert(quadtree_insert(tree, 2, 2, &val, NULL) == 1);
        assert(quadtree_insert(tree, 8, 8, &val, NULL) == 1);
        //assert(quadtree_insert(tree, 9, 9, &val, NULL) == 1);
        //assert(quadtree_insert(tree, 1, 2, &val, NULL) == 1);
        //assert(quadtree_insert(tree, 1, 1, &val, NULL) == 1);

        printf("\nBEFORE---\n");
        quadtree_walk(tree->root, ascent_draw, descent_draw);

        quadtree_node_t *node_the_node = quadtree_node_search(tree, 3.0, 8.0);
        quadtree_node_t *node_the_node2 = quadtree_node_search(tree, 8.0, 8.0);

        assert(quadtree_node_isleaf(node_the_node));
        assert(quadtree_node_isleaf(node_the_node2));

        /* Ensure tree condensed into a leaf properly */
        quadtree_point_t *point = malloc(sizeof(quadtree_point_t));
        if (point == NULL) {
                return;
        }
        point->x = 2;
        point->y = 2;
        for (i = 0; i < 1000; i++) {
                point->x += 0.0001;
                printf("\nMoving point (%f, %f) to (%f, %f)\n", node_the_node->point->x, node_the_node->point->y, point->x, point->y);
                assert(quadtree_move_leaf(tree, &node_the_node, point) == 1);
                point->y -= 0.0001;
                //printf("Moving point (%f, %f) to (%f, %f)\n", node_the_node2->point->x, node_the_node2->point->y, point->x, point->y);
                assert(quadtree_move_leaf(tree, &node_the_node2, point) == 1);
                assert(node_the_node->point != NULL);
                assert(node_the_node2->point != NULL);
        }

        printf("\nAFTER---\n");
        quadtree_walk(tree->root, ascent_draw, descent_draw);
}

static void
test_tree_condense_draw()
{
        int val = 10;
        quadtree_t *tree = quadtree_new(0, 0, 10, 10);

        /* Ensure tree has two leaves where I expect them. */
        assert(quadtree_insert(tree, 3, 8, &val, NULL) == 1);
        assert(quadtree_insert(tree, 2, 2, &val, NULL) == 1);
        assert(quadtree_insert(tree, 8, 8, &val, NULL) == 1);
        assert(quadtree_insert(tree, 9, 9, &val, NULL) == 1);
        assert(quadtree_insert(tree, 1, 2, &val, NULL) == 1);
        assert(quadtree_insert(tree, 1, 1, &val, NULL) == 1);

        quadtree_walk(tree->root, ascent_draw, descent_draw);
        /* Ensure tree condensed into a leaf properly */
        printf("\n\n\n\t NEW TREE\n\n");
        quadtree_clear_leaf(quadtree_node_search(tree, 1.0, 1.0));
        quadtree_clear_leaf(quadtree_node_search(tree, 1.0, 2.0));
        quadtree_clear_leaf(quadtree_node_search(tree, 2.0, 2.0));
        quadtree_walk(tree->root, ascent_draw, descent_draw);
}

static void
transfer_node(quadtree_t *from, quadtree_t *to, quadtree_point_t *old_point, quadtree_point_t *new_point)
{
        quadtree_node_t *move_this_node = quadtree_node_search(from, old_point->x, old_point->y);
        ASSERT_VERBOSE(move_this_node != NULL, from);
        void *val = quadtree_clear_leaf_with_condense(from, move_this_node);
        quadtree_insert(to, new_point->x, new_point->y, val, NULL);
}

static void
test_multiple_trees_transfer() {
        /* Tree A is responsible for left half and B for right half */
        quadtree_t *tree_A = quadtree_new(0, 0, 10, 10);
        quadtree_t *tree_B = quadtree_new(0, 0, 10, 10);
        const size_t total_nodes = 1000;
        quadtree_point_t points[total_nodes];

        const int val = 12;

        for (size_t i = 0; i < total_nodes; i++) {
                points[i].x = (double) ((double)rand()/RAND_MAX*10.0);
                points[i].y = (double) ((double)rand()/RAND_MAX*10.0);
                if (points[i].x <= 5)
                        quadtree_insert(tree_A, points[i].x, points[i].y, &val, NULL);
                else
                        quadtree_insert(tree_B, points[i].x, points[i].y, &val, NULL);
        }

        for (size_t i = 0; i < total_nodes; i++) {
                quadtree_point_t old_point = {points[i].x, points[i].y};

                points[i].y = (double) ((double)rand()/RAND_MAX*10.0);
                if (points[i].x <= 5) {
                        points[i].x = (double) (5.0 + (double)rand()/RAND_MAX*5.0);
                        transfer_node(tree_A, tree_B, &old_point, &points[i]);
                }
                else {
                        points[i].x = (double) ((double)rand()/RAND_MAX*5.0);
                        transfer_node(tree_B, tree_A, &old_point, &points[i]);
                }
        }
}

static void
test_subtree_transfer() {
        /* Tree A is responsible for left half and B for right half */
        quadtree_t *tree_A = quadtree_new(0, 0, 10, 10);
        quadtree_t *tree_B = quadtree_new(0, 0, 10, 10);
        const size_t total_nodes = 10;
        quadtree_point_t points[total_nodes];

        for (size_t i = 0; i < total_nodes; i++) {
                points[i].x = (double) ((double)rand()/RAND_MAX*10.0);
                points[i].y = (double) ((double)rand()/RAND_MAX*10.0);
                if (points[i].x <= 5)
                        quadtree_insert(tree_A, points[i].x, points[i].y, NULL, NULL);
                else
                        quadtree_insert(tree_B, points[i].x, points[i].y, NULL, NULL);
        }

        printf("BEFORE ______\n");
        printf("TREE A\n");
        quadtree_walk(tree_A->root, descent, ascent);
        printf("TREE B\n");
        quadtree_walk(tree_B->root, descent, ascent);
        printf("\n");
        printf("AFTER-=--==\n");
        printf("\n");
        quadtree_move_subtree(tree_B, quadtree_find_optimal_split_quad(tree_A));;
        printf("TREE A\n");
        quadtree_walk(tree_A->root, descent, ascent);
        printf("TREE B\n");
        quadtree_walk(tree_B->root, descent, ascent);
}



static void
test_weight(){
        int val = 10;
        int val2 = 42;

        quadtree_t *tree = quadtree_new(0, 0, 10, 10);
        quadtree_node_t *node;

        assert(quadtree_insert(tree, 4, 4, &val, &node) == 1);
        assert(quadtree_insert(tree, 3, 4, &val2, &node) == 1);

        /* Move from SW to NW */
        assert(quadtree_move_leaf(tree, &node, quadtree_point_new(3, 8)) == 1);

        assert(quadtree_insert(tree, 1, 1, &val, &node) == 1);
        assert(quadtree_insert(tree, 6, 0, &val, &node) == 1);
        assert(quadtree_insert(tree, 4, 6, &val, &node) == 1);
        assert(quadtree_insert(tree, 2.1, 2, &val, &node) == 1);
        assert(quadtree_insert(tree, 1, 1.2, &val, &node) == 1);
        assert(quadtree_insert(tree, 1, 1.5, &val, &node) == 1);

        node = quadtree_find_optimal_split_quad(tree);

        quadtree_free(tree);
}

static void
test_tree(){
        int val = 10;

        quadtree_t *tree = quadtree_new(0, 0, 10, 10);
        quadtree_node_t *node;
        quadtree_node_list_t *query_result;

        /* Test that quads can be reused. */
        assert(quadtree_insert(tree, 4, 4, &val, NULL) == 1);
        node = grab_first_node_from_query(tree, 2.5, 2.5, 2);
        assert (quadtree_clear_leaf(node) != NULL);
        /* New val for later sanity testing */
        int val2 = 42;
        assert(quadtree_insert(tree, 3, 4, &val2, NULL) == 1);

        /* Test that quads can be moved.
         * -- Lateral test from one quadrant to another under the same parent */

        /* Grab old node */
        node = grab_first_node_from_query(tree, 3, 4 ,1);
        assert(node->point->x == 3 && node->point->y == 4);
        /* Move from SW to NW */
        assert(quadtree_move_leaf(tree, &node, quadtree_point_new(3, 8)) == 1);

        node = grab_first_node_from_query(tree, 3, 8, 2);
        /* Check that query grabbed correct new node */
        assert(node->point->x == 3 && node->point->y == 8);
        assert(*(int *)node->key == 42);

        /* Check that old node is gone */
        query_result = quadtree_search_bounds(tree, 3, 4, 2);
        assert(query_result == NULL);
        quadtree_node_list_free(query_result);

        assert(quadtree_insert(tree, 1, 1, &val, NULL) == 1);
        assert(quadtree_insert(tree, 6, 0, &val, NULL) == 1);
        assert(quadtree_insert(tree, 6, 6, &val, NULL) == 1);
        assert(quadtree_insert(tree, 3.1, 4, &val, NULL) == 1);
        assert(quadtree_insert(tree, 3, 3, &val, NULL) == 1);
        assert(quadtree_insert(tree, 2, 1.2, &val, NULL) == 1);
        assert(quadtree_insert(tree, 2, 1, &val, NULL) == 1);
        assert(quadtree_insert(tree, 5, 1, &val, NULL) == 1);
        assert(quadtree_insert(tree, 1, 1.1, &val, NULL) == 1);
        assert(quadtree_insert(tree, 1, 2.1, &val, NULL) == 1);
        assert(quadtree_insert(tree, 1, 3.1, &val, NULL) == 1);
#if 0
        assert(quadtree_insert(tree, 0, 0, &val, NULL) == 0);
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

        query_result = quadtree_search_bounds_include_partial(tree, 2.5, 2.5, 5);
        quadtree_node_list_t *curr = query_result;
        printf("\nQuery results: for (2.5, 2.5) r=5:\n");
        while(curr != NULL && curr->node != NULL) {
                printf("(%lf, %lf) --> ", curr->node->point->x, curr->node->point->y);
                curr = curr->next;
        }
        printf("NULL\n");
        quadtree_node_list_free(query_result);

        query_result = quadtree_search_bounds(tree, 3, 8, 0.1);
        curr = query_result;
        printf("\nQuery results:\n");
        while(curr != NULL && curr->node != NULL) {
                printf("(%lf, %lf) --> ", curr->node->point->x, curr->node->point->y);
                curr = curr->next;
        }
        printf("NULL\n");
        quadtree_node_list_free(query_result);

        quadtree_free(tree);
}

static void
test_partial_coverage(){
        int val = 10;

        quadtree_t *tree = quadtree_new(0, 0, 10, 10);
        quadtree_node_list_t *query_result;

        assert(quadtree_insert(tree, 1, 1, &val, NULL) == 1);
        assert(quadtree_insert(tree, 3, 1, &val, NULL) == 1);
        quadtree_walk(tree->root, ascent, descent);

        query_result = quadtree_search_bounds(tree, 2.5, 2.5, 5);
        quadtree_node_list_t *curr = query_result;
        printf("\nQuery results: for (2.5, 2.5) r=5:\n");
        while(curr != NULL && curr->node != NULL) {
                printf("(%lf, %lf) --> ", curr->node->point->x, curr->node->point->y);
                curr = curr->next;
        }
        printf("NULL\n");
        quadtree_node_list_free(query_result);

        query_result = quadtree_search_bounds(tree, 2.5, 1, 1);
        curr = query_result;
        printf("\nQuery results:\n");
        while(curr != NULL && curr->node != NULL) {
                printf("(%lf, %lf) --> ", curr->node->point->x, curr->node->point->y);
                curr = curr->next;
        }
        printf("NULL\n");

        query_result = quadtree_search_bounds_include_partial(tree, 2.5, 1, 1);

        curr = query_result;
        printf("\nQuery results (partial):\n");
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

#define NUM_RAND_POINTS 1000
#define NUM_CLUSTERS_MAX 5
#define NUM_CLUSTERS_MIN 1
#define CLUSTER_SIZE_MAX 250
#define CLUSTER_SIZE_MIN 50
#define RADIUS_MAX 3
#define EXTRA_SPACE_ADJ 0.5
static void
test_rand_tree(){
        int i, num_clusters, cluster, num_elem_in_cluster, origin_x, origin_y, radius;
        int val = 10;
        double x,y, extra_space;

        quadtree_t *tree = quadtree_new(0, 0, 10, 10);

        //a few random points first
        for (i = 0; i < NUM_RAND_POINTS; i++) {
                x = (double) ((double)rand()/RAND_MAX*10.0);
                y = (double) ((double)rand()/RAND_MAX*10.0);
                quadtree_insert(tree, x, y, &val, NULL);
        }
        num_clusters = rand() % (NUM_CLUSTERS_MAX + 1 - NUM_CLUSTERS_MIN) + NUM_CLUSTERS_MIN;
        for (cluster = 0; cluster < num_clusters; cluster++){
                num_elem_in_cluster = rand() % (CLUSTER_SIZE_MAX + 1 - CLUSTER_SIZE_MIN) + CLUSTER_SIZE_MIN;
                radius = (double) ((double)rand()/RAND_MAX*RADIUS_MAX);
                origin_x = (double) ((double)rand()/RAND_MAX*10.0);
                origin_y = (double) ((double)rand()/RAND_MAX*10.0);
                for (i = 0; i < num_elem_in_cluster; i++) {
                        do {
                                x = (double) ((double)rand()/RAND_MAX*radius*2-radius+origin_x);
                                y = (double) ((double)rand()/RAND_MAX*radius*2-radius+origin_y);
                                extra_space = (double) ((double)rand()/RAND_MAX*EXTRA_SPACE_ADJ);
                                //if its more that that its not in a circle!
                        } while ((x-origin_x)*(x-origin_x) + (y-origin_y)*(y-origin_y) > radius * radius + extra_space*extra_space);
                        quadtree_insert(tree, x, y, &val, NULL);
                }
        }

        quadtree_walk(tree->root, ascent_draw, descent_draw);

        quadtree_node_list_t *query_result = quadtree_search_bounds_include_partial(tree, 4.9, 4.5, 2.9);
        quadtree_node_list_t *curr = query_result;
        while(curr != NULL && curr->node != NULL) {
                printf("Selected: %lf %lf\n", curr->node->point->x, curr->node->point->y);
                curr = curr->next;
        }
        quadtree_free(tree);
}

static void
test_leaf_move_stable(void)
{
        int val          = 10;
        int i;
        size_t test_size = 1000;
        quadtree_node_t *nodes[test_size];
        quadtree_t *tree = quadtree_new(0, 0, test_size, test_size);

        for (size_t i = 0; i < test_size; i++) {
                quadtree_insert(tree, i, i, &val, &nodes[i]);
        }

        for (size_t i = 0; i < test_size; i++) {
                quadtree_point_t point = {test_size - i, i};
                quadtree_move_leaf(tree, &nodes[i], &point);
        }

        quadtree_walk(tree->root, ascent_draw, descent_draw);

        for (size_t i = 0; i < test_size; i++) {
                assert(quadtree_node_isleaf(nodes[i]));
        }

        for (size_t i = 0; i < test_size; i++) {
                quadtree_point_t point = {i, test_size};
                quadtree_move_leaf(tree, &nodes[i], &point);
        }

        printf("\nBEFORE---\n");
        quadtree_walk(tree->root, ascent_draw, descent_draw);

        for (size_t i = 0; i < test_size; i++) {
                assert(quadtree_node_isleaf(nodes[i]));
        }
}


int
main(int argc, const char *argv[]){
        /* printf("\nquadtree_t: %ld\n", sizeof(quadtree_t)); */
        /* printf("quadtree_node_t: %ld\n", sizeof(quadtree_node_t)); */
        /* printf("quadtree_bounds_t: %ld\n", sizeof(quadtree_bounds_t)); */
        /* printf("quadtree_point_t: %ld\n", sizeof(quadtree_point_t)); */
        /* test(tree); */
        /* test(node); */
        /* test(bounds); */
        /* test(points); */
        /* test(tree_condense); */
        /* test(partial_coverage); */
        /* test(tree_condense_draw); */
        //test(leaf_move);

        test(weight);
        test(multiple_trees_transfer);
        test(subtree_transfer);

        /* test(rand_tree); */
        /* test(leaf_move_complex); */
        //test(leaf_move_stable);
}
