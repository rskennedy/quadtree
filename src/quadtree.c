#include "quadtree.h"
#include <stdio.h>
#include <assert.h>

/* private prototypes */
static int
split_node_(quadtree_t *tree, quadtree_node_t *node, quadtree_node_t **fill_this_in);

static int
insert_(quadtree_t* tree, quadtree_node_t *root, quadtree_point_t *point, void *key, quadtree_node_t **node_p);

static int
node_contains_(quadtree_node_t *outer, quadtree_point_t *it);

static quadtree_node_t *
get_quadrant_(quadtree_node_t *root, quadtree_point_t *point);

static void
search_bounds_(quadtree_node_t *root, quadtree_bounds_t *box, quadtree_node_list_t **result);

static void
search_bounds_include_partial_(quadtree_node_t *root, quadtree_bounds_t *box, quadtree_node_list_t **result);

static int
bounds_contains_point_(quadtree_bounds_t *bounds, quadtree_point_t *point);

static void
condense_parent(quadtree_t *tree,quadtree_node_t *parent);

void descent_(quadtree_node_t *node){
  if(node->bounds != NULL)
    //printf("{ nw.x:%f, nw.y:%f, se.x:%f, se.y:%f }: ", node->bounds->nw->x,
    printf("%f %f %f %f - ", node->bounds->nw->x,
      node->bounds->nw->y, node->bounds->se->x, node->bounds->se->y);
  if(node->point != NULL)
    printf("%lf:%lf\n", node->point->x, node->point->y);
}

void ascent_(quadtree_node_t *node){
  printf("\n");
}

/* private implementations */
static int
node_contains_(quadtree_node_t *outer, quadtree_point_t *it) {
        return outer->bounds != NULL
                && outer->bounds->nw->x <= it->x
                && outer->bounds->nw->y >= it->y
                && outer->bounds->se->x >= it->x
                && outer->bounds->se->y <= it->y;
}

/* private implementations */
static inline int
bounds_contains_bounds_(quadtree_bounds_t *inside, quadtree_bounds_t *outside)
{
        return outside != NULL
                && outside->nw->x <= inside->nw->x
                && outside->nw->y >= inside->nw->y
                && outside->se->x >= inside->se->x
                && outside->se->y <= inside->se->y;
}

static inline int
bounds_overlap_bounds_(quadtree_bounds_t *inside, quadtree_bounds_t *outside)
{
        return ! (outside == NULL
                  || (inside->nw->x > outside->se->x || outside->nw->x > inside->se->x) 
                  || (inside->nw->y < outside->se->y || outside->nw->y < inside->se->y));
}

static inline int
bounds_contains_point_(quadtree_bounds_t *bounds, quadtree_point_t *point)
{
        return bounds != NULL
                && point != NULL
                && bounds->nw->x <= point->x
                && bounds->nw->y >= point->y
                && bounds->se->x >= point->x
                && bounds->se->y <= point->y;
}

static void
elision_(void* key){}

static void
reset_node_(quadtree_t *tree, quadtree_node_t *node){
        if(tree->key_free != NULL) {
                quadtree_node_reset(node, tree->key_free);
        } else {
                quadtree_node_reset(node, elision_);
        }
}

static quadtree_node_t *
get_quadrant_(quadtree_node_t *root, quadtree_point_t *point) {
        if(node_contains_(root->nw, point)) return root->nw;
        if(node_contains_(root->ne, point)) return root->ne;
        if(node_contains_(root->sw, point)) return root->sw;
        if(node_contains_(root->se, point)) return root->se;
        return NULL;
}

static inline void
swap_points(quadtree_node_t *node, quadtree_node_t *new_node)
{
        quadtree_point_t *tmp = node->point;
        node->point = new_node->point;
        new_node->point = tmp;
}

static inline void
swap_coordinates(quadtree_node_t *node, quadtree_node_t *new_node)
{
        coordinate_t tmp = node->coord;
        node->coord = new_node->coord;
        new_node->coord = tmp;
}

static inline void
swap_bounds(quadtree_node_t *node, quadtree_node_t *new_node)
{
        quadtree_bounds_t *tmp = node->bounds;
        node->bounds = new_node->bounds;
        new_node->bounds = tmp;
}


static inline void
swap_parents(quadtree_t *tree, quadtree_node_t *node, quadtree_node_t *new_node)
{
        quadtree_node_t *node_parent = node->parent;
        quadtree_node_t *new_node_parent = new_node->parent;
        if (node_parent == NULL && new_node_parent ==NULL)
                assert(0);
        /*
        if (node_parent == NULL)
                node_parent = tree->root; 
        if (new_node_parent == NULL)
                new_node_parent = tree->root; 
        */

        if (node_parent != NULL) {
                switch(node->coord) {
                        case NW:
                                node_parent->nw = new_node;
                                break;
                        case NE:
                                node_parent->ne = new_node;
                                break;
                        case SW:
                                node_parent->sw = new_node;
                                break;
                        case SE:
                                node_parent->se = new_node;
                                break;
                        case NO_COORDINATE:
                                break;
                        default:
                                printf("Whose demon node is this?\n");
                                assert(0);
                }
        }

        if (new_node_parent != NULL) {       
                switch(new_node->coord) {
                        case NW:
                                new_node_parent->nw = node;
                                break;
                        case NE:
                                new_node_parent->ne = node;
                                break;
                        case SW:
                                new_node_parent->sw = node;
                                break;
                        case SE:
                                new_node_parent->se = node;
                                break;
                        case NO_COORDINATE:
                                break;
                        default:
                                printf("Whose demon node is this?\n");
                                assert(0);
                }
        }

        if (node_parent == NULL)
                tree->root = new_node;
        else if (new_node_parent == NULL)
                tree->root = node;

        /* node->parent = new_node->parent; */
        node->parent = new_node;
        new_node->parent = node_parent;

        node->nw->parent = new_node;
        node->ne->parent = new_node;
        node->sw->parent = new_node;
        node->se->parent = new_node;
}

static inline void
swap_children_count(quadtree_node_t *node, quadtree_node_t *new_node)
{
        unsigned int tmp = node->children_cnt;
        node->children_cnt = new_node->children_cnt;
        new_node->children_cnt = tmp;

        tmp = node->weight;
        node->weight = new_node->weight;
        new_node->weight = tmp;
}

static inline void
swap_children(quadtree_node_t *node, quadtree_node_t *new_node)
{
        quadtree_node_t *nw = node->nw;
        quadtree_node_t *ne = node->ne;
        quadtree_node_t *sw = node->sw;
        quadtree_node_t *se = node->se;

        node->nw = new_node->nw;
        node->ne = new_node->ne;
        node->sw = new_node->sw;
        node->se = new_node->se;

        new_node->nw = nw;
        new_node->ne = ne;
        new_node->sw = sw;
        new_node->se = se;
}

static void
swap_node_details(quadtree_t *tree, quadtree_node_t *node, quadtree_node_t *new_node)
{
        /* swap parents first */
        swap_parents(tree, node, new_node);

        swap_coordinates(node, new_node);
        swap_points(node, new_node);
        swap_bounds(node, new_node);
        swap_children(node, new_node);
        swap_children_count(node, new_node);
}

static int
split_node_(quadtree_t *tree, quadtree_node_t *node, quadtree_node_t **fill_this_in){
        quadtree_node_t *nw;
        quadtree_node_t *ne;
        quadtree_node_t *sw;
        quadtree_node_t *se;
        quadtree_point_t *old;
        void *key;

        double x  = node->bounds->nw->x;
        double y  = node->bounds->nw->y;
        double hw = node->bounds->width / 2;
        double hh = node->bounds->height / 2;

        //minx,   miny,       maxx,       maxy
        if(!(nw = quadtree_node_with_bounds(x,      y - hh,     x + hw,     y))) return 0;
        if(!(ne = quadtree_node_with_bounds(x + hw, y - hh,     x + hw * 2, y))) return 0;
        if(!(sw = quadtree_node_with_bounds(x,      y - hh * 2, x + hw,     y - hh))) return 0;
        if(!(se = quadtree_node_with_bounds(x + hw, y - hh * 2, x + hw * 2, y - hh))) return 0;

        nw->coord = NW;
        nw->parent = node;
        node->nw = nw;

        ne->coord = NE;
        ne->parent = node;
        node->ne = ne;

        sw->coord = SW;
        sw->parent = node;
        node->sw = sw;

        se->coord = SE;
        se->parent = node;
        node->se = se;

        old = node->point;
        key   = node->key;
        node->weight = 1;
        node->point = NULL;
        node->key   = NULL;

        quadtree_node_t *new_node = NULL;
        int ret = insert_(tree, node, old, key, &new_node);
        if (ret > 0) {
               assert(new_node != NULL);
               swap_node_details(tree, node, new_node);
               *fill_this_in = new_node;
        }
        return ret;

}


static quadtree_point_t*
find_(quadtree_node_t* node, double x, double y) {
        if(!node){
                return NULL;
        }
        if(quadtree_node_isleaf(node)){
                if(node->point->x == x && node->point->y == y)
                        return node->point;
        } else if(quadtree_node_ispointer(node)){
                quadtree_point_t test;
                test.x = x;
                test.y = y;
                return find_(get_quadrant_(node, &test), x, y);
        }

        return NULL;
}

static quadtree_node_t*
find_node_from_point_(quadtree_node_t* node, double x, double y) {
        if(!node){
                return NULL;
        }
        if(quadtree_node_isleaf(node)){
                if(node->point->x == x && node->point->y == y)
                        return node;
        } else if(quadtree_node_ispointer(node)){
                quadtree_point_t test;
                test.x = x;
                test.y = y;
                return find_node_from_point_(get_quadrant_(node, &test), x, y);
        }

        return NULL;
}

static void
extract_all_(quadtree_node_t *root, quadtree_node_list_t **result)
{
        if (root == NULL) {
                return;
        }
        else if(quadtree_node_isleaf(root)){
                quadtree_node_list_add(result, root);
        } else {
                extract_all_(root->nw, result);
                extract_all_(root->ne, result);
                extract_all_(root->sw, result);
                extract_all_(root->se, result);
        }
}

static void
extract_all_within_bounds_(quadtree_node_t *root, quadtree_bounds_t *box, quadtree_node_list_t **result)
{
        if (root == NULL) {
                return;
        }
        else if(quadtree_node_isleaf(root)){
                printf("Checking if (%lf, %lf) is within (%lf, %lf) (%lf, %lf)\n", root->point->x, root->point->y, box->nw->x, box->nw->y, box->se->x, box->se->y);
                if (bounds_contains_point_(box, root->point))
                        quadtree_node_list_add(result, root);
        } else {
                extract_all_within_bounds_(root->nw, box, result);
                extract_all_within_bounds_(root->ne, box, result);
                extract_all_within_bounds_(root->sw, box, result);
                extract_all_within_bounds_(root->se, box, result);
        }
}

static void
eval_quad_(quadtree_node_t *root, quadtree_bounds_t *box, quadtree_node_list_t **result)
{
        if (bounds_contains_bounds_(root->bounds, box)) {
                extract_all_(root, result);
        } else if (bounds_contains_point_(box, root->point)) {
                search_bounds_(root, box, result);
        }
}

static void
eval_quad_partial_(quadtree_node_t *root, quadtree_bounds_t *box, quadtree_node_list_t **result)
{
        if (root == NULL)
                return;

        /* Check if completely inside */
        if (bounds_contains_bounds_(root->bounds, box)) {
                extract_all_(root, result);
        /* If overlapping a part of it explore child quads */
        } else if (bounds_overlap_bounds_(root->bounds, box)) {
                if (quadtree_node_ispointer(root)) {
                        eval_quad_partial_(root->nw, box, result);
                        eval_quad_partial_(root->ne, box, result);
                        eval_quad_partial_(root->sw, box, result);
                        eval_quad_partial_(root->se, box, result);
                } else {
                        extract_all_within_bounds_(root, box, result);
                }
        /* If its a leaf */
        } else if(quadtree_node_isleaf(root) && bounds_contains_point_(box, root->point)) {
                quadtree_node_list_add(result, root);
        }
}

static void
search_bounds_(quadtree_node_t *root, quadtree_bounds_t *box, quadtree_node_list_t **result)
{
        if (root == NULL) {
                return;
        }
        printf("checking root w bounds (%lf, %lf) || (%lf, %lf) \n", root->bounds->nw->x, root->bounds->nw->y, root->bounds->se->x, root->bounds->se->y);
        if(quadtree_node_isleaf(root)){
                if (bounds_contains_point_(box, root->point)) {
                        quadtree_node_list_add(result, root);
                }
        } else if (quadtree_node_ispointer(root)) {
                /* recursion occurs within eval_quad() */
                eval_quad_(root->nw, box, result);
                eval_quad_(root->ne, box, result);
                eval_quad_(root->sw, box, result);
                eval_quad_(root->se, box, result);
        }
}

static void
search_bounds_include_partial_(quadtree_node_t *root, quadtree_bounds_t *box, quadtree_node_list_t **result)
{
        if (root == NULL) {
                return;
        }
        printf("Checking root w partial bounds (%lf, %lf) || (%lf, %lf) \n", root->bounds->nw->x, root->bounds->nw->y, root->bounds->se->x, root->bounds->se->y);
        if(quadtree_node_isleaf(root)){
                if (bounds_contains_point_(box, root->point)) {
                        quadtree_node_list_add(result, root);
                }
        } else if (quadtree_node_ispointer(root)) {
                /* recursion occurs within eval_quad() */
                eval_quad_partial_(root->nw, box, result);
                eval_quad_partial_(root->ne, box, result);
                eval_quad_partial_(root->sw, box, result);
                eval_quad_partial_(root->se, box, result);
        }
}

static void
inc_parent_cnt(quadtree_node_t *node)
{
        node->parent->children_cnt += 1;
        assert(node->parent->children_cnt <= 4);
}

static void
dec_parent_cnt(quadtree_node_t *node)
{
        assert(node->parent->children_cnt != 0);
        node->parent->children_cnt -= 1;
}

static void
dec_parent_cnt_with_weight(quadtree_node_t *node)
{
        dec_parent_cnt(node);
        if (quadtree_node_isleaf(node)) {
                while (node->parent != NULL) {
                        node->parent->weight--;
                        node = node->parent;
                }
        }
}

/* cribbed from the google closure library. */
static int
insert_(quadtree_t* tree, quadtree_node_t *root, quadtree_point_t *point, void *key, quadtree_node_t **node_p) {
        if(quadtree_node_isempty(root)){
                root->point = point;
                root->key   = key;
                if (root->parent != NULL) {
                        inc_parent_cnt(root);
                }
                if (node_p != NULL) {
                        *node_p = root;
                }

                return 1; /* normal insertion flag */
        } else if(quadtree_node_isleaf(root)){
                quadtree_node_t *fill_this_in = NULL;
                if (root->point->x == point->x && root->point->y == point->y){
                        reset_node_(tree, root);
                        root->point = point;
                        root->key   = key;
                        if (node_p != NULL)
                                *node_p = root;
                        return 2; /* replace insertion flag */
                } else {
                        if (!split_node_(tree, root, &fill_this_in)){
                                printf("Failed to split node\n");
                                return 0; /* failed insertion flag */
                        }

                        return insert_(tree, fill_this_in, point, key, node_p);
                }
        } else if(quadtree_node_ispointer(root)){
                quadtree_node_t* quadrant = get_quadrant_(root, point);
                if (quadrant == NULL) {
                        printf("Point: (%lf, %lf)\n", point->x, point->y);
                        printf("Boundaries: NW (%lf, %lf),  SE (%lf, %lf)\n", root->bounds->nw->x, root->bounds->nw->y, root->bounds->se->x, root->bounds->se->y);
                        printf("NW bounds: (%lf, %lf) (%lf, %lf)\n", root->nw->bounds->nw->x,root->nw->bounds->nw->y,root->nw->bounds->se->x,root->nw->bounds->se->y);
                        printf("NE bounds: (%lf, %lf) (%lf, %lf)\n", root->ne->bounds->nw->x,root->ne->bounds->nw->y,root->ne->bounds->se->x,root->ne->bounds->se->y);
                        printf("SW bounds: (%lf, %lf) (%lf, %lf)\n", root->sw->bounds->nw->x,root->sw->bounds->nw->y,root->sw->bounds->se->x,root->sw->bounds->se->y);
                        printf("SE bounds: (%lf, %lf) (%lf, %lf)\n", root->se->bounds->nw->x,root->se->bounds->nw->y,root->se->bounds->se->x,root->se->bounds->se->y);
                        return 0;
                }
                return insert_(tree, quadrant, point, key, node_p);
        }
        return 0;
}


/* public */
quadtree_t*
quadtree_new(double minx, double miny, double maxx, double maxy) {
        quadtree_t *tree;
        if(!(tree = rte_malloc("tree",sizeof(*tree),0)))
                return NULL;
        tree->root = quadtree_node_with_bounds(minx, miny, maxx, maxy);
        if(!(tree->root))
                return NULL;
        tree->key_free = NULL;
        tree->length = 0;
        return tree;
}

/* public */
quadtree_node_list_t*
quadtree_node_list_new(quadtree_node_t *node) {
        quadtree_node_list_t *new = rte_malloc("list",sizeof(quadtree_node_list_t),0);
        if (new == NULL) {
                return NULL;
        }
        new->node = node;
        new->next = NULL;
        return new;
}

void
quadtree_node_list_free(quadtree_node_list_t *list)
{
        if (list == NULL) {
                return;
        }

        quadtree_node_list_t *next;
        quadtree_node_list_t *curr = list;

        while (curr != NULL) {
                next = curr->next;
                rte_free(curr);
                curr = next;
        }
}

void
quadtree_node_list_add(quadtree_node_list_t **list_p, quadtree_node_t *node)
{
        quadtree_node_list_t *new = quadtree_node_list_new(node);
        new->next = *list_p;
        *list_p = new;
}

int
quadtree_insert(quadtree_t *tree, double x, double y, void *key, quadtree_node_t **node_p) {
        quadtree_point_t *point;
        int insert_status;

        /* backup container for node pointer if user doesn't provide one */
        quadtree_node_t *node;
        if (node_p == NULL) {
                node_p = &node;
        }

        if(!(point = quadtree_point_new(x, y))) return -1;
        if(!node_contains_(tree->root, point)){
                quadtree_point_free(point);
                return -2;
        }

        if(!(insert_status = insert_(tree, tree->root, point, key, node_p))){
                quadtree_point_free(point);
                return -3;
        }
        if (insert_status == 1) {
                tree->length++;
                quadtree_node_t *child = *node_p;
                while (child->parent != NULL) {
                        child->parent->weight++;
                        child = child->parent;
                }
        }

        return insert_status;
}

quadtree_point_t*
quadtree_search(quadtree_t *tree, double x, double y) {
        return find_(tree->root, x, y);
}

quadtree_node_t*
quadtree_node_search(quadtree_t *tree, double x, double y) {
        return find_node_from_point_(tree->root, x, y);
}

quadtree_node_list_t*
quadtree_search_bounds(quadtree_t *tree, double x, double y, double radius)
{
        //TODO: error checking on valid bounds for map

        /* Build box */
        quadtree_point_t *top_left = quadtree_point_new(x - radius, y + radius);
        if (top_left == NULL) {
                return NULL;
        }
        quadtree_point_t *bottom_right = quadtree_point_new(x + radius, y - radius);
        if (bottom_right == NULL) {
                quadtree_point_free(top_left);
                return NULL;
        }
        quadtree_bounds_t *box = quadtree_bounds_new();
        if (box == NULL) {
                quadtree_point_free(top_left);
                quadtree_point_free(bottom_right);
                return NULL;
        }
        box->nw = top_left;
        box->se = bottom_right;

        /* Will contain list of matching nodes */
        quadtree_node_list_t *result = NULL;
        search_bounds_(tree->root, box, &result);
        return result;
}

quadtree_node_list_t*
quadtree_search_bounds_include_partial(quadtree_t *tree, double x, double y, double radius)
{
        //TODO: error checking on valid bounds for map

        /* Build box */
        quadtree_point_t *top_left = quadtree_point_new(x - radius, y + radius);
        if (top_left == NULL) {
                return NULL;
        }
        quadtree_point_t *bottom_right = quadtree_point_new(x + radius, y - radius);
        if (bottom_right == NULL) {
                quadtree_point_free(top_left);
                return NULL;
        }
        quadtree_bounds_t *box = quadtree_bounds_new();
        if (box == NULL) {
                quadtree_point_free(top_left);
                quadtree_point_free(bottom_right);
                return NULL;
        }
        box->nw = top_left;
        box->se = bottom_right;

        /* Will contain list of matching nodes */
        quadtree_node_list_t *result = NULL;
        search_bounds_include_partial_(tree->root, box, &result);
        return result;
}

void
quadtree_free(quadtree_t *tree) {
        if(tree->key_free != NULL) {
                quadtree_node_free(tree->root, tree->key_free);
        } else {
                quadtree_node_free(tree->root, elision_);
        }
        rte_free(tree);
}

void
quadtree_walk(quadtree_node_t *root, void (*descent)(quadtree_node_t *node),
                void (*ascent)(quadtree_node_t *node)) {
        (*descent)(root);
        if(root->nw != NULL) quadtree_walk(root->nw, descent, ascent);
        if(root->ne != NULL) quadtree_walk(root->ne, descent, ascent);
        if(root->sw != NULL) quadtree_walk(root->sw, descent, ascent);
        if(root->se != NULL) quadtree_walk(root->se, descent, ascent);
        (*ascent)(root);
}

/* Call this on a leaf.
 * Assumes all siblings are empty. */
static void
condense_(quadtree_t *tree, quadtree_node_t *node)
{
        assert(!quadtree_node_ispointer(node));
        //int cleanup_cnt = 0;
        quadtree_node_t *parent = node->parent;
        quadtree_node_t *gparent = parent->parent;

        assert(bounds_contains_bounds_(node->bounds, parent->bounds));

        if (parent->nw != node) {
                assert(quadtree_node_isempty(parent->nw));
        } else {
                parent->nw = NULL;
        }
        if (parent->ne != node) {
                assert(quadtree_node_isempty(parent->ne));
        } else {
                parent->ne = NULL;
        }
        if (parent->sw != node) {
                assert(quadtree_node_isempty(parent->sw));
        } else {
                parent->sw = NULL;
        }
        if (parent->se != node) {
                assert(quadtree_node_isempty(parent->se));
        } else {
                parent->se = NULL;
        }

        node->coord = parent->coord;

        switch(parent->coord) {
        case NW:
                gparent->nw = node;
                break;
        case NE:
                gparent->ne = node;
                break;
        case SW:
                gparent->sw = node;
                break;
        case SE:
                gparent->se = node;
                break;
        case NO_COORDINATE:
                break;
        default:
                printf("Whose demon node is this?\n");
                assert(0);
        }

        /* Swap bounds */
        quadtree_bounds_t *tmp = node->bounds;
        node->bounds = parent->bounds;
        parent->bounds = tmp;

        quadtree_node_free(parent, elision_);

        node->parent = gparent;
        node->nw = NULL;
        node->ne = NULL;
        node->sw = NULL;
        node->se = NULL;

        if (gparent == NULL) {
                tree->root = node;
        } else if (gparent->children_cnt == 1) {
                condense_parent(tree, gparent);
        }
}


static void
condense_parent(quadtree_t *tree,quadtree_node_t *parent)
{
        if (parent == NULL) //|| parent->parent == NULL || parent->parent->parent == NULL)
                return;

        quadtree_node_t *last_child = NULL;
        /* Parent replaces last child. */
        if (!quadtree_node_isempty(parent->nw)) last_child = parent->nw;
        if (!quadtree_node_isempty(parent->ne)) last_child = parent->ne;
        if (!quadtree_node_isempty(parent->sw)) last_child = parent->sw;
        if (!quadtree_node_isempty(parent->se)) last_child = parent->se;
        assert(last_child != NULL);
        if (quadtree_node_ispointer(last_child)) {
                return;
        }

        condense_(tree, last_child);


        /* WORKS */
        /* parent->point = last_child->point; */
        /* parent->key = last_child->key; */

        /* parent->children_cnt = 0; */
        /* free(parent->nw); */
        /* free(parent->ne); */
        /* free(parent->sw); */
        /* free(parent->se); */
        /* parent->nw = NULL; */
        /* parent->ne = NULL; */
        /* parent->sw = NULL; */
        /* parent->se = NULL; */

        /* DOES NOT NECESSARILY WORK */
        /* if (parent->parent != NULL && parent->parent->children_cnt == 1) { */
        /*         condense_parent(parent->parent); */
        /* } */
}

/*
 * Reset a leaf node into an empty node.
 * Returns key.
 */
void *
quadtree_clear_leaf(quadtree_node_t *node)
{
        void *key = node->key;
        rte_free(node->point);
        node->point = NULL;
        node->key = NULL;

        return key;
}
/*
 * Reset a leaf node into an empty node.
 * Returns key.
 */
void *
quadtree_clear_leaf_with_condense(quadtree_t *tree, quadtree_node_t *node)
{
        void *key = node->key;
        rte_free(node->point);
        node->point = NULL;
        node->key = NULL;
        if (node->parent != NULL) {
                dec_parent_cnt_with_weight(node);
                if (node->parent->children_cnt == 1) {
                        /* printf("Would have condensed parent, but did not\n"); */
                        condense_parent(tree, node->parent);
                }
        }
        return key;
}

void
recalc_weight(quadtree_node_t *node, unsigned int weight_diff)
{
        while (node->parent != NULL) {
                node->parent->weight -= weight_diff;
                node = node->parent;
        }
}

/*
 * Cuts out subtree and recalcs the weight of the ancestor nodes.
 * Don't call this on root.
 */
void
quadtree_unlink_subtree(quadtree_t *destination_tree, quadtree_node_t *subtree_root)
{
        assert(subtree_root->parent != NULL);

        unsigned int weight_diff = subtree_root->weight;

        quadtree_node_t *filler_node = quadtree_node_new();
        filler_node->parent          = subtree_root->parent;
        filler_node->coord           = subtree_root->coord;
        assert(quadtree_node_isempty(filler_node));

        switch(subtree_root->coord) {
                case NW:
                        subtree_root->parent->nw = filler_node;
                        break;
                case NE:
                        subtree_root->parent->ne = filler_node;
                        break;
                case SW:
                        subtree_root->parent->sw = filler_node;
                        break;
                case SE:
                        subtree_root->parent->se = filler_node;
                        break;
                default:
                        printf("UGHGHGHG\n");
                        break;
        }

        subtree_root->parent         = NULL;

        dec_parent_cnt(filler_node);
        recalc_weight(filler_node, weight_diff);
        if (filler_node->parent->children_cnt == 1) {
                condense_parent(destination_tree, filler_node->parent);
        }
}

int quadtree_insert_subtree(quadtree_t *destination_tree, quadtree_node_t *subtree_root)
{
        quadtree_node_list_t *list;
        /* extract_all_(subtree_root, &list); */
        search_bounds_(subtree_root, subtree_root->bounds, &list);
        for (; list; list = list->next) {
                if (list->node->point == NULL) {
                        printf("This shouldnt hapepn the list thingy is null\n");
                        continue;
                }
                quadtree_insert(destination_tree, list->node->point->x, list->node->point->y, list->node->key, NULL);
        }
        quadtree_node_free(subtree_root, rte_free);
        quadtree_node_list_free(list);
        return 0;
}

int
quadtree_move_subtree(quadtree_t *destination_tree, quadtree_node_t *subtree_root)
{
        /* TODO: Pay attention to the ordering */

        /* Clear and condense without destroying subtree root */
        /* recalculate weight */
        quadtree_unlink_subtree(destination_tree, subtree_root);
        quadtree_insert_subtree(destination_tree, subtree_root);

        return 0;
}

int
quadtree_move_leaf(quadtree_t *tree, quadtree_node_t **node_p, quadtree_point_t *point)
{
        void *key;
        quadtree_node_t *node = *node_p;

        if (tree == NULL || node == NULL || point == NULL)
                return -1;

        if(!quadtree_node_isleaf(node)){
                printf("\nOh nononononono\n");
                //quadtree_walk(tree->root, ascent_, descent_);
        }

        assert(quadtree_node_isleaf(node));

        key = quadtree_clear_leaf_with_condense(tree,node);
        //printf("\nAfter removing node---\n");
        //quadtree_walk(tree->root, ascent_, descent_);

        /* quadtree_walk(tree->root, ascent_, descent_); */

        /* FIXME: For now, root of tree is starting point. Eventually should check parent first. */
        int ret = quadtree_insert(tree, point->x, point->y, key, node_p);
        return ret;
        //return insert_(tree, tree->root, point, key);
}

quadtree_node_t *
quadtree_find_max_weight_child(quadtree_node_t *node) {
        quadtree_node_t *max_child = node->nw;
        if (node->ne->weight > max_child->weight)
                max_child = node->ne;
        if (node->sw->weight > max_child->weight)
                max_child = node->sw;
        if (node->se->weight > max_child->weight)
                max_child = node->se;
        return max_child;
}


quadtree_node_t *
quadtree_find_optimal_split_quad(quadtree_t *tree) {
        double optimal_weight_ratio = 0.5;
        quadtree_node_t *current;

        // Quad where child_weight / tree->root->weight ~= optimal_weight

        current = tree->root;
        while ((double)current->weight / tree->root->weight > optimal_weight_ratio) {
                current = quadtree_find_max_weight_child(current);
        }
        return current;
}
