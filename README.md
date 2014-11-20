Native Node.JS Quadtree
=======================
A native implementation of a QuadTree for Node.JS. This was put together to be used as the backing data structure for high-performance collision detection in 2D.

The implementation is largely in pure C. Most data is allocated up front, so there are very few uses of malloc or free. The QuadTree is kept in memory as an array, so the implementation should be largely cache friendly. Further development to this will attempt to reduce use of malloc and free even further.

The API is currently very simple: when node_quadtree is required, the exported functions are QuadTree and Rect, both requiring new to create. A QuadTree object has add, which takes a Rect and any object, and intersect, which just takes a rect and returns an array of objects where the rect key has the colliding rect and obj has the object associated with that rect. Rect has the attributes x, y, w, h.


Performance
===========
Currently the set of performance tests in test.js indicate that, on average, intersection tests take 2-3 microseconds (2000-3000 nanoseconds) meaning about 333,333 intersection tests can be performed every millisecond. This scales to 1000 objects in the QuadTree spread evenly along the diagonal from the top left down to the bottom right for a 2048x2048 pixel tree. When the tree is only 1024x1024, it can take up to 16+ microseconds per intersection test, but that is due to the tree becoming tightly packed with the 1000 objects, causing each intersection test to need to iterate over a large number of items all the way at the bottom of the tree. Typical usage should not see the tree getting so tightly packed with items, and should see the 2-3 microsecond performance.

Testing for how fast adding elements is has not yet been done.

Removing items from the quadtree is not yet implemented, so there can be no performance data on it yet.