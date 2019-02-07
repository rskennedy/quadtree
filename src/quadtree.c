#include "quadtree.h"
#include <stdio.h>

/* private prototypes */
static int
split_node_(quadtree_t *tree, quadtree_node_t *node);

static int
insert_(quadtree_t* tree, quadtree_node_t *root, quadtree_point_t *point, void *key);

static int
node_contains_(quadtree_node_t *outer, quadtree_point_t *it);

static quadtree_node_t *
get_quadrant_(quadtree_node_t *root, quadtree_point_t *point);

static void
search_bounds_(quadtree_node_t *root, quadtree_bounds_t *box, quadtree_node_list_t **result);

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

static int
split_node_(quadtree_t *tree, quadtree_node_t *node){
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

  node->nw = nw;
  node->ne = ne;
  node->sw = sw;
  node->se = se;

  old = node->point;
  key   = node->key;
  node->point = NULL;
  node->key   = NULL;

  return insert_(tree, node, old, key);
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

static void
extract_all_(quadtree_node_t *root, quadtree_node_list_t *result)
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
eval_quad_(quadtree_node_t *root, quadtree_bounds_t *box, quadtree_node_list_t *result)
{
        if (bounds_contains_bounds_(root->bounds, box)) {
                extract_all_(root, result);
        } else if (bounds_contains_point_(box, root->point)) {
                search_bounds_(root, box, result);
        }
}

static void
search_bounds_(quadtree_node_t *root, quadtree_bounds_t *box, quadtree_node_list_t **result)
{
        if (root == NULL) {
                return;
        }
        printf("Checking root w bounds (%lf, %lf) || (%lf, %lf) \n", root->bounds->nw->x, root->bounds->nw->y, root->bounds->se->x, root->bounds->se->y);
        if(quadtree_node_isleaf(root)){
                if (bounds_contains_point_(box, root->point)) {
                        quadtree_node_list_add(result, root);
                }
        } else if (quadtree_node_ispointer(root)) {
                /* Recursion occurs within eval_quad() */
                eval_quad_(root->nw, box, result);
                eval_quad_(root->ne, box, result);
                eval_quad_(root->sw, box, result);
                eval_quad_(root->se, box, result);
        }
}

/* cribbed from the google closure library. */
static int
insert_(quadtree_t* tree, quadtree_node_t *root, quadtree_point_t *point, void *key) {
  if(quadtree_node_isempty(root)){
    root->point = point;
    root->key   = key;
    return 1; /* normal insertion flag */
  } else if(quadtree_node_isleaf(root)){
    if(root->point->x == point->x && root->point->y == point->y){
      reset_node_(tree, root);
      root->point = point;
      root->key   = key;
      return 2; /* replace insertion flag */
    } else {
      if(!split_node_(tree, root)){
        return 0; /* failed insertion flag */
      }
      return insert_(tree, root, point, key);
    }
  } else if(quadtree_node_ispointer(root)){
    quadtree_node_t* quadrant = get_quadrant_(root, point);
    return quadrant == NULL ? 0 : insert_(tree, quadrant, point, key);
  }
  return 0;
}


/* public */
quadtree_t*
quadtree_new(double minx, double miny, double maxx, double maxy) {
  quadtree_t *tree;
  if(!(tree = malloc(sizeof(*tree))))
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
        quadtree_node_list_t *new = malloc(sizeof(quadtree_node_list_t));
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
                free(curr);
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
quadtree_insert(quadtree_t *tree, double x, double y, void *key) {
  quadtree_point_t *point;
  int insert_status;

  if(!(point = quadtree_point_new(x, y))) return 0;
  if(!node_contains_(tree->root, point)){
    quadtree_point_free(point);
    return 0;
  }

  if(!(insert_status = insert_(tree, tree->root, point, key))){
    quadtree_point_free(point);
    return 0;
  }
  if (insert_status == 1) tree->length++;
  return insert_status;
}

quadtree_point_t*
quadtree_search(quadtree_t *tree, double x, double y) {
  return find_(tree->root, x, y);
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

void
quadtree_free(quadtree_t *tree) {
  if(tree->key_free != NULL) {
    quadtree_node_free(tree->root, tree->key_free);
  } else {
    quadtree_node_free(tree->root, elision_);
  }
  free(tree);
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
