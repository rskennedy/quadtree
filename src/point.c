#include "quadtree.h"

quadtree_point_t*
quadtree_point_new(double x, double y) {
        quadtree_point_t* point;
        if (!(point = malloc(sizeof(*point))))
                // if(!(point = rte_malloc("point", sizeof(*point), 0)))
                return NULL;
        point->x = x;
        point->y = y;
        return point;
}

void
quadtree_point_free(quadtree_point_t* point) {
        free(point);
        // rte_free(point);
}
