#include "BridgeEngine.h"
#include <stdio.h>

static int expect(int condition, const char *message)
{
	if (!condition) fprintf(stderr, "%s\n", message);
	return condition;
}

int main(void)
{
	bapi_rect_t base = {0, 0, 10, 10};
	bapi_rect_t touching_rect = {10, 0, 5, 5};
	bapi_rect_t overlapping_rect = {9, 0, 5, 5};
	bapi_circle_t touching_circle = bapi_circle(4, 0, 2);
	bapi_circle_t base_circle = bapi_circle(0, 0, 2);
	bapi_rect_t circle_touching_rect = {0, 0, 4, 4};
	bapi_circle_t circle_touching_rect_edge = bapi_circle(-2, 2, 2);

	int result = expect(!bapi_rect_intersects(base, touching_rect),
					"edge-touching rectangles do not intersect") &&
			 expect(bapi_rect_intersects(base, overlapping_rect),
					"positive-area rectangle overlap intersects") &&
			 expect(bapi_circle_intersects_circle(base_circle, touching_circle),
					"touching circles intersect") &&
			 expect(bapi_circle_intersects_rect(circle_touching_rect_edge, circle_touching_rect),
					"circle touching rectangle edge intersects");

	bapi_rect_t cases[][2] = {
		{base, touching_rect},
		{base, overlapping_rect},
		{base, {-20, 0, 5, 5}},
	};
	for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); index++) {
		result = result && expect(bapi_collision_aabb(cases[index][0], cases[index][1]) ==
								   bapi_rect_intersects(cases[index][0], cases[index][1]),
								"AABB alias matches rectangle intersection");
	}

	return result ? 0 : 1;
}
