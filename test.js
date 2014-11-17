var qt = require("./index");

var tree = new qt.QuadTree(1024, 1024);
var r = new qt.Rect(50, 0, 1024, 1024);
console.log(qt);
console.log(tree);
console.log(r);

console.log(tree.add);
var testobj = {foo: "bar"};
for(var i = 0; i < 15; i++) {
    // tree.add(new qt.Rect(5 * i, 5 * i, 100 + 5 * i, 100 + 5 * i), {foo: "bar" + i});
    tree.add(new qt.Rect(5 * i, 5 * i, 100 + 5 * i, 100 + 5 * i), testobj);
}
var total = 0;
var times = 100000;
for(var i = 0; i < times; i++) {
    var t1 = process.hrtime()
    var rects = tree.intersecting(new qt.Rect(45, 45, 1, 1));
    var t2 = process.hrtime();
    var t1nano = t1[0] * 1000000 + t1[1] / 1000;
    var t2nano = t2[0] * 1000000 + t2[1] / 1000;
    total += t2nano - t1nano;
}
console.log("intersecting took avg " + (total / times) + " microseconds");
console.log((1000000 / (total / times)) + " intersections can be made per second");
for(var i = 0; i < rects.length; i++) {
    console.log(rects[i]);
}
// tree.add(new qt.Rect(0, 0, 100, 100), testobj);