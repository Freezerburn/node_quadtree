#include <node.h>
#include <v8.h>

#include <math.h>

#define DEFAULT_MAX_LAYERS 10
#define DEFAULT_NUM_ENTRY 10
#define nfroml(vname,l) unsigned vname=0;for(unsigned i=0;i<l;i++){vname+=pow(4,i);}
#define qnch(i,n) ((i)*3+(n))

using namespace v8;

static Persistent<FunctionTemplate> qtree_wrap;
static Persistent<FunctionTemplate> rect_wrap;

struct qrectI {
    float x, y, w, h;
};
typedef struct qrectI qrect;
struct qnentryI {
    qrect r;
    Persistent<Value> d;
};
typedef struct qnentryI qnentry;
struct qnodeI {
    unsigned i, l;/*index,layer*/
    qrect r;
    qnentry *d;/*data*/
    unsigned cd, md;/*current data,max data*/
};
typedef struct qnodeI qnode;
struct qtreeI {
    unsigned ml;/*max layers for this qtree*/
    qnode ns[1];/*array of qtree nodes*/
};
typedef struct qtreeI qtree;
struct qcolresultsI {
    unsigned ce, me;
    qnentry *e;
};
typedef struct qcolresultsI qcolresults;

void qcolresultsd(qcolresults *r) {
    for(unsigned i = 0; i < r->ce; i++) {
        r->e[i].d.~Persistent();
    }
    free(r->e);
    free(r);
}
qcolresults* qcolresultsn(unsigned me) {
    qcolresults *ret = (qcolresults*)malloc(sizeof(qcolresults));
    ret->e = (qnentry*)malloc(sizeof(qnentry) * me);
    ret->ce = 0;
    ret->me = me;
    for(unsigned i = 0; i < me; i++) {
        ret->e[i].d = Persistent<Value>();
    }
    return ret;
}

qrect* rectn(float x, float y, float w, float h) {
    qrect *r = (qrect*)malloc(sizeof(qrect));
    r->x = x;
    r->y = y;
    r->w = w;
    r->h = h;
    return r;
}

int recteq(qrect *r1, qrect *r2) {
    float fx = fabs(r1->x - r2->x);
    float fy = fabs(r1->y - r2->y);
    float fw = fabs(r1->w - r2->w);
    float fh = fabs(r1->h - r2->h);
    // printf("fx:%.3f;fy:%.3f;fw:%.3f;fh:%.3f\n", fx, fy, fw, fh);
    if(fx < 0.0001f && fy < 0.0001f && fw < 0.0001f && fh < 0.0002f)return 1;
    return 0;
}

void rect_printf(qrect *r) {
    printf("Rect{x:%.3f;y:%.3f;w:%.3f;h:%.3f}", r->x, r->y, r->w, r->h);
}
void rect_printfn(qrect *r) {
    rect_printf(r);
    printf("\n");
}

int rect_intersect(qrect *r1, qrect *r2) {
    // float ulx = fmaxf(r1->x, r2->x);
    // float uly = fmaxf(r1->y, r2->y);
    // float lrx = fminf(r1->x + r1->w, r2->x + r2->w);
    // float lry = fminf(r1->y + r1->h, r2->y + r2->h);
    // return ulx <= lrx && uly <= lry ? 1 : 0;
    if(r1->x + r1->w < r2->x)return 0;
    else if(r1->x > r2->x + r2->w)return 0;
    else if(r1->y + r1->h < r2->y)return 0;
    else if(r1->y > r2->y + r2->h)return 0;
    return 1;
}

void qnoded(qnode *n) {
    // printf("Freeing node data %d\n", n->i);
    free(n->d);
}
void qtreed(qtree *qt) {
    nfroml(nd,qt->ml + 1)
    // printf("nd: %d\n", nd);
    for(unsigned i = 0; i < nd; i++) {
        qnoded(qt->ns + i);
    }
    // printf("")
    // free(qt->ns);
    // printf("Freeing qtree.");
    free(qt);
}

void qnode_printf(qnode *n) {
    printf("QuadTreeNode{index:%d;layer:%d;num items:%d;max items:%d;", n->i, n->l, n->cd, n->md);
    rect_printf(&n->r);
}
void qnode_printfn(qnode *n) {
    qnode_printf(n);
    printf("\n");
}

void qnodewhn(qnode *n, unsigned i, unsigned l, qrect *r, unsigned md) {
    n->i = i;
    n->l = l;
    n->r = *r;
    n->d = (qnentry*)malloc(sizeof(qnentry) * md);
    n->cd = 0;
    n->md = md;
}

void qtree_initroot(qnode *n, int w, int h, unsigned nentry) {
    n->i = 0;
    n->l = 0;
    n->r.w = w;
    n->r.h = h;
    n->d = (qnentry*)malloc(sizeof(qnentry) * nentry);
    n->cd = 0;
    n->md = nentry;
}
qtree* qtreen(int w, int h, unsigned ml) {
    nfroml(nd,ml)
    qtree *ret = (qtree*)malloc(sizeof(qtree) + sizeof(qnode) * nd);
    ret->ml = ml - 1;
    memset(ret->ns, 0, sizeof(qnode) * nd);
    qtree_initroot(ret->ns, w, h, DEFAULT_NUM_ENTRY);
    return ret;
}

void qnodeadd(qnode *n, unsigned ml, unsigned l, qrect *r, Persistent<Value> d) {
    // printf("Current layer: %d, max layer: %d\n", l, ml);
    // printf("qnoadeadd Current Node: ");
    // qnode_printfn(n);
    // printf("Rect for element being added: ");
    // rect_printfn(r);
    if(!rect_intersect(&n->r, r))return;

    if(!n->cd) {
        int noch = 1;/*no children?*/
        if(l != ml) {
            for(unsigned i = 1; i <= 4; i++) {
                unsigned chn = qnch(n->i,i);
                // printf("parent %d has child %d at %d\n", n->i, i, n->i + chn);
                // printf("child value: 0x%lx\n", *(unsigned long*)(n + chn));
                if(*(unsigned long*)(n + chn) != 0) {
                    noch = 0;
                    break;
                }
            }
        }
        // printf("Has no children?: %d\n", noch);

        if(noch) {
            /*w. node, convert to b. node*/
            // printf("Current node White. Making Black.\n");
            n->d[n->cd].r = *r;
            n->d[n->cd++].d = d;
        }
        else {
            /*g. node, recurse*/
            // printf("Current node Gray. Recursing.\n");
            for(unsigned i = 1; i <= 4; i++) {
                qnodeadd(n + qnch(n->i,i), ml, l+1, r, d);
            }
        }
    }
    else {
        // printf("Current node Black.\n");
        if(n->cd == n->md) {
            /*b. node full*/
            if(l == ml) {
                /*max depth, increase array size*/
                // printf("Black node full, realloc.\n");
                unsigned nm = n->md * 2;
                // printf("New max: %d\n", n->md);
                qnentry *tmp = (qnentry*)malloc(sizeof(qnentry) * nm);
                memcpy(tmp, n->d, sizeof(qnentry) * n->md);
                free(n->d);
                n->d = tmp;
                n->md = nm;
                // printf("realloc done. setting values.\n");
                n->d[n->cd].r = *r;
                n->d[n->cd++].d = d;
                // printf("values set.\n");
            }
            else {
                // printf("Black node full, making Gray and sending elements to children.\n");
                for(unsigned i = 1; i <= 4; i++) {
                    unsigned ni = qnch(n->i,i);
                    qnode *nn = n + ni;
                    qrect nr;
                    nr.x = (i % 2) == 1 ? n->r.x : n->r.x + n->r.w / 2.0f;
                    nr.y = (i % 2) == 1 ? n->r.x : n->r.y + n->r.h / 2.0f;
                    nr.w = n->r.w / 2.0f;
                    nr.h = n->r.h / 2.0f;
                    qnodewhn(nn, n->i + ni, n->l + 1, &nr, DEFAULT_NUM_ENTRY);
                    // qnode_printfn(nn);
                    qnodeadd(nn, ml, l + 1, r, d);
                    for(unsigned j = 0; j < n->cd; j++) {
                        qnodeadd(nn, ml, l + 1, &n->d[j].r, n->d[j].d);
                    }
                }
                // free(n->d);
                n->cd = 0;
            }
        }
        else {
            // printf("Adding element to Black node.\n");
            n->d[n->cd].r = *r;
            n->d[n->cd++].d = d;
            // printf("Black node after add: ");
            // qnode_printfn(n);
        }
    }
}
void qnode_remove(qnode *n, unsigned ml, unsigned l, Persistent<Value> d) {
    for(unsigned i = 0; i < n->cd; i++) {
        if(n->d[i].d->StrictEquals(d)) {
            if(i + 1 == n->cd) {
                n->d[i].d.Dispose();
                n->d[i].d.Clear();
                n->cd--;
            }
            else {
                n->d[i].d.Dispose();
                n->d[i].d.Clear();
                memmove(n->d + i, n->d + i + 1, (n->cd - i) * sizeof(qnentry));
                n->cd--;
            }
        }
    }
}
void qnode_colliding(qnode *n, qrect *r, qcolresults *results, unsigned l, unsigned ml) {
    // printf("coll current node: ");
    // qnode_printfn(n);
    // printf("coll rectangle to collide: ");
    // rect_printfn(r);
    if(!rect_intersect(&n->r, r))return;

    for(unsigned i = 0; i < n->cd; i++) {
        // printf("coll checking element rect: ");
        // rect_printfn(&n->d[i].r);
        if(rect_intersect(&n->d[i].r, r)) {
            int skip = 0;
            // printf("coll passed in rect: ");
            // rect_printfn(r);
            // printf("coll with rect: ");
            // rect_printfn(&n->d[i].r);
            // printf("coll data: %s\n", *String::Utf8Value(n->d[i].d));
            for(unsigned j = 0; j < results->ce; j++) {
                if(recteq(&n->d[i].r, &results->e[j].r) && results->e[j].d->StrictEquals(n->d[i].d)) {
                    skip = 1;
                    break;
                }
            }
            if(!skip) {
                if(results->ce == results->me) {
                    // printf("Need to realloc results array.\n");
                    results->me *= 2;
                    results->e = (qnentry*)realloc(results->e, sizeof(qnentry) * results->me);
                }
                results->e[results->ce].r.x = n->d[i].r.x;
                results->e[results->ce].r.y = n->d[i].r.y;
                results->e[results->ce].r.w = n->d[i].r.w;
                results->e[results->ce].r.h = n->d[i].r.h;
                results->e[results->ce++].d = n->d[i].d;
            }
        }
    }

    if(l != ml) {
        unsigned nch = qnch(n->i, 1);
        if(*(unsigned long*)(n + nch) != 0) {
            qnode_colliding(n + nch, r, results, l + 1, ml);
        }
        nch = qnch(n->i, 2);
        if(*(unsigned long*)(n + nch) != 0) {
            qnode_colliding(n + nch, r, results, l + 1, ml);
        }
        nch = qnch(n->i, 3);
        if(*(unsigned long*)(n + nch) != 0) {
            qnode_colliding(n + nch, r, results, l + 1, ml);
        }
        nch = qnch(n->i, 4);
        if(*(unsigned long*)(n + nch) != 0) {
            qnode_colliding(n + nch, r, results, l + 1, ml);
        }
    }
}
void qtreeadd(qtree *qt, qrect *r, Persistent<Value> d) {
    // rect_printfn(r);
    qnodeadd(qt->ns, qt->ml, 0, r, d);
}
void qtreerem(qtree *qt, Persistent<Value> d) {
    qnode_remove(qt->ns, qt->ml, 0, d);
}
// static qcolresults *global_results = NULL;
qcolresults* qtree_colliding(qtree *qt, qrect *r) {
    // if(global_results == NULL) {
    //     global_results = qcolresultsn(100);
    // }
    qcolresults *results = qcolresultsn(100);
    qnode_colliding(qt->ns, r, results, 0, qt->ml);
    return results;
}

void js_weak_qrect(Persistent<Value> value, void *data) {
    HandleScope scope;
    free(data);
    value.ClearWeak();
    value.Dispose();
    value.Clear();
}

Handle<Value> js_rectn(const Arguments &args) {
    if(!args.IsConstructCall()) {
        return ThrowException(Exception::TypeError(
            String::New("Use the new operator to create instances of a QuadTree Rect.")));
    }
    if(args.Length() < 4) {
        return ThrowException(Exception::TypeError(
            String::New("Need an x, y, width, and height for the Rect!")));
    }

    HandleScope scope;
    float x = (float)args[0]->NumberValue();
    float y = (float)args[1]->NumberValue();
    float w = (float)args[2]->NumberValue();
    float h = (float)args[3]->NumberValue();
    qrect *r = rectn(x, y, w, h);
    Persistent<Object> ret = Persistent<Object>::New(args.This());
    ret->SetPointerInInternalField(0, r);
    ret.MakeWeak(r, js_weak_qrect);
    ret.MarkIndependent();
    return scope.Close(args.This());
}

Handle<Value> js_rect_xget(Local<String> prop, const AccessorInfo &info) {
    HandleScope scope;
    qrect *r = (qrect*)info.This()->GetPointerFromInternalField(0);
    return scope.Close(Number::New(r->x));
}
Handle<Value> js_rect_yget(Local<String> prop, const AccessorInfo &info) {
    HandleScope scope;
    qrect *r = (qrect*)info.This()->GetPointerFromInternalField(0);
    return scope.Close(Number::New(r->y));
}
Handle<Value> js_rect_wget(Local<String> prop, const AccessorInfo &info) {
    HandleScope scope;
    qrect *r = (qrect*)info.This()->GetPointerFromInternalField(0);
    return scope.Close(Number::New(r->w));
}
Handle<Value> js_rect_hget(Local<String> prop, const AccessorInfo &info) {
    HandleScope scope;
    qrect *r = (qrect*)info.This()->GetPointerFromInternalField(0);
    return scope.Close(Number::New(r->h));
}

void js_weak_qtree(Persistent<Value> value, void *data) {
    qtree *qt = (qtree*)data;
    qtreed(qt);
}

Handle<Value> js_qtreen(const Arguments &args) {
    if(!args.IsConstructCall()) {
        return ThrowException(Exception::TypeError(
            String::New("Use the new operator to create instances of a QuadTree.")));
    }
    if(args.Length() < 2) {
        return ThrowException(Exception::TypeError(
            String::New("Need a width and height for the QuadTree!")));
    }

    HandleScope scope;
    int w = args[0]->Int32Value();
    int h = args[1]->Int32Value();
    int ml = args[2]->IsUndefined() ? DEFAULT_MAX_LAYERS : args[2]->Int32Value();
    qtree *qt = qtreen(w, h, ml);
    // printf("qt addr new: 0x%lx\n", (unsigned long)qt);
    Persistent<Object> ret = Persistent<Object>::New(args.This());
    ret->SetPointerInInternalField(0, qt);
    ret.MakeWeak(qt, js_weak_qtree);
    ret.MarkIndependent();
    return scope.Close(args.This());
}

void js_weak_storedobj(Persistent<Value> value, void *data) {
    value.ClearWeak();
    value.Dispose();
    value.Clear();
}

Handle<Value> js_qtree_add(const Arguments &args) {
    if(args.Length() < 2) {
        return ThrowException(Exception::TypeError(
            String::New("Need a rect and object to add to the QuadTree!")));
    }

    HandleScope scope;
    qtree *qt = (qtree*)args.This()->GetPointerFromInternalField(0);
    // printf("qt addr add: 0x%lx\n", (unsigned long)qt);
    Local<Object> js_rect = args[0]->ToObject();
    // printf("Got js_rect object.\n");
    Persistent<Value> d = Persistent<Value>::New(args[1]);
    d.MakeWeak(NULL, js_weak_storedobj);
    // printf("Made new persistent from argument 1.\n");
    // d.MarkIndependent();
    // printf("Persistent marked as independent.\n");
    float x = (float)js_rect->Get(String::New("x"))->NumberValue();
    // printf("Got x.\n");
    float y = (float)js_rect->Get(String::New("y"))->NumberValue();
    // printf("Got y.\n");
    float w = (float)js_rect->Get(String::New("w"))->NumberValue();
    // printf("Got w.\n");
    float h = (float)js_rect->Get(String::New("h"))->NumberValue();
    // printf("qt add Rect{x:%f,y:%f,w:%f,h:%f\n", x, y, w, h);
    qrect r = {x, y, w, h};
    // printf("Doing qt add!\n");
    qtreeadd(qt, &r, d);
    // printf("Finished add! Returning this.\n");
    return scope.Close(args.This());
}

Handle<Value> js_qtree_coll(const Arguments &args) {
    if(args.Length() < 4) {
        return ThrowException(Exception::TypeError(
            String::New("Need x, y, w, h to find collisions with!")));
    }

    HandleScope scope;
    qtree *qt = (qtree*)args.This()->GetPointerFromInternalField(0);
    // Local<Object> js_rect = args[0]->ToObject();
    // float x = (float)js_rect->Get(String::New("x"))->NumberValue();
    // float y = (float)js_rect->Get(String::New("y"))->NumberValue();
    // float w = (float)js_rect->Get(String::New("w"))->NumberValue();
    // float h = (float)js_rect->Get(String::New("h"))->NumberValue();
    float x = args[0]->NumberValue();
    float y = args[1]->NumberValue();
    float w = args[2]->NumberValue();
    float h = args[3]->NumberValue();
    qrect r = {x, y, w, h};
    qcolresults *results = qtree_colliding(qt, &r);
    // printf("num results: %d\n", results->ce);
    // for(unsigned i = 0; i < results->ce; i++) {
    //     rect_printfn(&results->e[i].r);
    // }
    Local<Array> ret = Array::New(results->ce);
    if(results->ce > 1000) {
        printf("!!!!! %d results!\n", results->ce);
    }
    for(unsigned i = 0; i < results->ce; i++) {
        Local<Number> x = Number::New(results->e[i].r.x);
        Local<Number> y = Number::New(results->e[i].r.y);
        Local<Number> w = Number::New(results->e[i].r.w);
        Local<Number> h = Number::New(results->e[i].r.h);
        Handle<Value> argv[] = {x, y, w, h};
        Local<Value> rect = rect_wrap->GetFunction()->NewInstance(4, argv);
        Local<Object> pair = Object::New();
        pair->Set(String::New("rect"), rect);
        pair->Set(String::New("obj"), results->e[i].d);
        ret->Set(i, pair);
    }
    qcolresultsd(results);
    return scope.Close(ret);
}

extern "C" void init(Handle<Object> target) {
    Local<FunctionTemplate> qtree_templ = FunctionTemplate::New(js_qtreen);
    qtree_wrap = Persistent<FunctionTemplate>::New(qtree_templ);
    qtree_wrap->InstanceTemplate()->SetInternalFieldCount(1);
    qtree_wrap->SetClassName(String::NewSymbol("native_qtree"));
    NODE_SET_PROTOTYPE_METHOD(qtree_wrap, "add", js_qtree_add);
    NODE_SET_PROTOTYPE_METHOD(qtree_wrap, "intersecting", js_qtree_coll);
    target->Set(String::NewSymbol("QuadTree"), qtree_wrap->GetFunction());

    Local<FunctionTemplate> rect_templ = FunctionTemplate::New(js_rectn);
    rect_wrap = Persistent<FunctionTemplate>::New(rect_templ);
    rect_wrap->InstanceTemplate()->SetInternalFieldCount(1);
    rect_wrap->SetClassName(String::NewSymbol("native_qrect"));
    rect_wrap->InstanceTemplate()->SetAccessor(String::New("x"), js_rect_xget);
    rect_wrap->InstanceTemplate()->SetAccessor(String::New("y"), js_rect_yget);
    rect_wrap->InstanceTemplate()->SetAccessor(String::New("w"), js_rect_wget);
    rect_wrap->InstanceTemplate()->SetAccessor(String::New("h"), js_rect_hget);
    target->Set(String::NewSymbol("Rect"), rect_wrap->GetFunction());
}

NODE_MODULE(node_quadtree, init)