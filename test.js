var qt = require("./index");

function hr2micro(hr) {
    return hr[0] * 1000000 + hr[1] / 1000;
}

var w = 2048, h = 2048;
var addw = 1024, addh = 1024;
var times = 1000, intersect_times = times * 100;
var addperloop = times / 50;
var xpertime = w / times, ypertime = h / times;
var rw = 10, rh = 10;
var tree = new qt.QuadTree(w,h);
var addobj = {foo: "bar"};

var objs = [];
for(var i = 0; i < 10; i++) {
    var newobj = {name: "test" + i};
    var xloc = Math.random() * w, yloc = Math.random() * h;
    newobj.rect = new qt.Rect(xloc, yloc, 10, 20);
    tree.add(newobj.rect, newobj);
    objs.push(newobj);
}
console.log(tree.intersecting(0, 0, w, h));
for(var i = 0; i < objs.length; i++) {
    tree.remove(objs[i]);
    console.log(tree.intersecting(0, 0, w, h));
}

// for(var i = 0; i < times / 10; i++) {
//     console.log("Doing performance testing for QuadTree of size " + w + ", " + h);
//     for(var j = 0; j < times; j+=addperloop) {
//         for(var k = 0; k < addperloop; k++) {
//             tree.add(new qt.Rect(xpertime * (j + k), ypertime * (j + k), rw, rh), addobj);
//         }
//         var time = 0;
//         for(var k = 0; k < intersect_times; k++) {
//             var xloc = Math.random() * w, yloc = Math.random() * h;
//             var t1 = process.hrtime();
//             var results = tree.intersecting(xloc, yloc, rw, rh);
//             var t2 = process.hrtime();
//             time += hr2micro(t2) - hr2micro(t1);
//             // if(k % 1000 == 0)
//             //     global.gc();

//             // if(results.length > 15) {
//             //     console.log("LONG LENGTH DATA: " + results.length);
//             //     console.log(new qt.Rect(xloc, yloc, rw, rh));
//             //     for(var z = 0; z < results.length; z++) {
//             //         console.log("x:" + results[z].x + ";y:" + results[z].y +
//             //             ";w:" + results[z].w + ";h:" + results[z].h);
//             //         rectcolltest(results[z], results[z]);
//             //         console.log(results[z]);
//             //     }
//             // }
//         }
//         console.log("QuadTree with " + (j + addperloop) + " objects took avg " + (time / intersect_times)
//             + " microseconds for intersects.");
//     }
//     w += addw;
//     h += addh;
//     xpertime = w / times;
//     ypertime = h / times;
//     tree = new qt.QuadTree(w,h);
// }
// tree.add(new qt.Rect(0, 0, 100, 100), testobj);