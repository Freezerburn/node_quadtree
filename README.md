Native Node.JS Quadtree
=======================
A native implementation of a QuadTree for Node.JS. This was put together to be used as the backing data structure for high-performance collision detection in 2D.

The implementation is largely in pure C. Most data is allocated up front, so there are very few uses of malloc or free. The QuadTree is kept in memory as an array, so the implementation should be largely cache friendly. Further development to this will attempt to reduce use of malloc and free even further.

The API is currently very simple: when node_quadtree is required, the exported functions are QuadTree and Rect, both requiring new to create. A QuadTree object has add, which takes a Rect and any object, and intersect, which just takes a rect and returns an array of objects where the rect key has the colliding rect and obj has the object associated with that rect. Rect has the attributes x, y, w, h.