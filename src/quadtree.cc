#include <node.h>
#include <v8.h>
#include <nan.h>

#include <math.h>

#define DEFAULT_MAX_LAYERS 10
#define DEFAULT_NUM_ENTRY 10
#define nfroml(vname,l) unsigned vname=0;for(unsigned i=0;i<l;i++){vname+=pow(4,i);}
#define qnch(i,n) ((i)*3+(n))

using namespace v8;

static Nan::Persistent<FunctionTemplate> qtree_wrap;
static Nan::Persistent<FunctionTemplate> rect_wrap;

struct qrectI {
    float x, y, w, h;
};
typedef struct qrectI qrect;
struct qnentryI {
    qrect r;
    Nan::Persistent<Object> d;
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
        new ((void*)&ret->e[i].d) Nan::Persistent<Object>();
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
    if(n->d != NULL) {
        for(unsigned i = 0; i < n->cd; i++) {
            n->d[i].d.Reset();
            n->d[i].d.~Persistent();
        }
        for (unsigned i = n->cd, end = n->md; i < end; i++) {
            n->d[i].d.~Persistent();
        }
        free(n->d);
    }
}
void qtreed(qtree *qt) {
    nfroml(nd,qt->ml + 1)
    // printf("nd: %d\n", nd);
    for(unsigned i = 0; i < nd; i++) {
        qnoded(qt->ns + i);
    }
    // printf("")
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
    n->d = NULL;
    n->cd = 0;
    n->md = md;
}

void qtree_initroot(qnode *n, int w, int h, unsigned nentry) {
    n->i = 0;
    n->l = 0;
    n->r.w = w;
    n->r.h = h;
    n->d = (qnentry*)malloc(sizeof(qnentry) * nentry);
    for (unsigned i = 0; i < nentry; i++) {
        new ((void*)&n->d[i].d) Nan::Persistent<Object>();
    }
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

void qnodeadd(qnode *n, unsigned ml, unsigned l, qrect *r, Nan::Persistent<Object> *d) {
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
            if(n->d == NULL) {
                // printf("Allocating array for data... ");
                n->d = (qnentry*)malloc(sizeof(qnentry) * n->md);
                for (unsigned j = 0; j < n->md; j++) {
                    new ((void*)&n->d[j].d) Nan::Persistent<Object>();
                }
                // printf("done.\n");
            }
            n->d[n->cd].r = *r;
            // printf("Resetting persistent... ");
            n->d[n->cd++].d.Reset(*d);
            // printf("done.\n");
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
                for (unsigned j = n->md, end = n->md * 2; j < end; j++) {
                    new ((void*)&tmp[j].d) Nan::Persistent<Object>();
                }
                memcpy(tmp, n->d, sizeof(qnentry) * n->md);
                free(n->d);
                n->d = tmp;
                n->md = nm;
                // printf("realloc done. setting values.\n");
                n->d[n->cd].r = *r;
                n->d[n->cd++].d.Reset(*d);
                // printf("values set.\n");
            }
            else {
                // printf("Black node full, making Gray and sending elements to children.\n");
                for(unsigned i = 1; i <= 4; i++) {
                    unsigned ni = qnch(n->i,i);
                    qnode *nn = n + ni;
                    qrect nr;
                    nr.x = (i % 2) == 1 ? n->r.x : n->r.x + n->r.w / 2.0f;
                    nr.y = i < 3 ? n->r.y : n->r.y + n->r.h / 2.0f;
                    nr.w = n->r.w / 2.0f;
                    nr.h = n->r.h / 2.0f;
                    // rect_printfn(&nr);
                    qnodewhn(nn, n->i + ni, n->l + 1, &nr, DEFAULT_NUM_ENTRY);
                    // qnode_printfn(nn);
                    qnodeadd(nn, ml, l + 1, r, d);
                    for(unsigned j = 0; j < n->cd; j++) {
                        qnodeadd(nn, ml, l + 1, &n->d[j].r, &n->d[j].d);
                    }
                }
                free(n->d);
                n->d = NULL;
                n->cd = 0;
                n->md = DEFAULT_NUM_ENTRY;
            }
        }
        else {
            // printf("Adding element to Black node.\n");
            n->d[n->cd].r = *r;
            // printf("Resetting persistent... ");
            n->d[n->cd++].d.Reset(*d);
            // printf("done.\n");
            // printf("Black node after add: ");
            // qnode_printfn(n);
        }
    }
}
void qnode_remove(qnode *n, unsigned ml, unsigned l, qrect *r, Local<Value> d) {
    if(!rect_intersect(&n->r, r))return;

    for(unsigned i = 0; i < n->cd; i++) {
        if(rect_intersect(&n->d[i].r, r)) {
            if(n->d[i].d.operator==(d)) {
                if(i + 1 == n->cd) {
                    n->d[i].d.Reset();
                    n->cd--;
                }
                else {
                    for(unsigned j = i; j < n->cd - 1; j++) {
                        n->d[j].d.Reset(n->d[j + 1].d);
                        n->d[j].r = n->d[j + 1].r;
                    }
                    n->d[n->cd - 1].d.Reset();
                    n->cd--;
                }

                if(n->cd == 0) {
                    // printf("Destruct node start... ");
                    for (unsigned j = 0, end = n->md; j < end; j++) {
                        n->d[j].d.~Persistent();
                    }
                    // printf("done.\n");
                    free(n->d);
                    n->d = NULL;
                    n->md = DEFAULT_NUM_ENTRY;
                }
                else if(n->cd == n->md / 4) {
                    qnentry *tmp = (qnentry*)malloc(sizeof(qnentry) * n->cd * 2);
                    // printf("Compact node start... ");
                    for (unsigned j = n->cd, end = n->md; j < end; j++) {
                        n->d[j].d.~Persistent();
                    }
                    // printf("done.\n");
                    memcpy(tmp, n->d, sizeof(qnentry) * n->cd);
                    free(n->d);
                    n->d = tmp;
                    n->md = n->cd * 2;
                }
            }
        }
    }

    if(l != ml) {
        unsigned nch = qnch(n->i, 1);
        if(*(unsigned long*)(n + nch) != 0) {
            qnode_remove(n + nch, ml, l + 1, r, d);
        }
        nch = qnch(n->i, 2);
        if(*(unsigned long*)(n + nch) != 0) {
            qnode_remove(n + nch, ml, l + 1, r, d);
        }
        nch = qnch(n->i, 3);
        if(*(unsigned long*)(n + nch) != 0) {
            qnode_remove(n + nch, ml, l + 1, r, d);
        }
        nch = qnch(n->i, 4);
        if(*(unsigned long*)(n + nch) != 0) {
            qnode_remove(n + nch, ml, l + 1, r, d);
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
                if(recteq(&n->d[i].r, &results->e[j].r) && results->e[j].d.operator==(n->d[i].d)) {
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
                results->e[results->ce++].d.Reset(n->d[i].d);
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
void qtreeadd(qtree *qt, qrect *r, Nan::Persistent<Object> *d) {
    // printf("qtreeadd before\n");
    qnodeadd(qt->ns, qt->ml, 0, r, d);
    // printf("qtreeadd after\n");
}
void qtreerem(qtree *qt, qrect *r, Local<Value> d) {
    qnode_remove(qt->ns, qt->ml, 0, r, d);
}
qcolresults* qtree_colliding(qtree *qt, qrect *r) {
    qcolresults *results = qcolresultsn(100);
    qnode_colliding(qt->ns, r, results, 0, qt->ml);
    return results;
}

void js_weak_qrect(const Nan::WeakCallbackInfo<qrect> &data) {
    // printf("freeing qrect... ");
    free(data.GetParameter());
    // printf("done.\n");
}

NAN_METHOD(js_rectn) {
    Nan::EscapableHandleScope();
    if(!info.IsConstructCall()) {
        Nan::ThrowTypeError("Use the new operator to create instances of a QuadTree Rect.");
    }
    if(info.Length() < 4) {
        Nan::ThrowTypeError("Need an x, y, width, and height for the Rect!");
    }

    float x = (float)info[0]->NumberValue();
    float y = (float)info[1]->NumberValue();
    float w = (float)info[2]->NumberValue();
    float h = (float)info[3]->NumberValue();
    // printf("rect: x:%f, y:%f, w:%f, h:%f\n", x, y, w, h);
    qrect *r = rectn(x, y, w, h);
    Nan::SetInternalFieldPointer(info.This(), 0, r);
    Nan::Persistent<Object> ret(info.This());
    ret.SetWeak(r, js_weak_qrect, WeakCallbackType::kParameter);
    ret.MarkIndependent();
    info.GetReturnValue().Set(info.This());
}

NAN_PROPERTY_GETTER(js_rect_xget) {
    qrect *r = (qrect*)Nan::GetInternalFieldPointer(info.This(), 0);
    // printf("qrect retrieving x:%f\n", r->x);
    info.GetReturnValue().Set(Nan::New<Number>(r->x));
}
NAN_PROPERTY_GETTER(js_rect_yget) {
    qrect *r = (qrect*)Nan::GetInternalFieldPointer(info.This(), 0);
    // printf("qrect retrieving y:%f\n", r->y);
    info.GetReturnValue().Set(Nan::New<Number>(r->y));
}
NAN_PROPERTY_GETTER(js_rect_wget) {
    qrect *r = (qrect*)Nan::GetInternalFieldPointer(info.This(), 0);
    // printf("qrect retrieving w:%f\n", r->w);
    info.GetReturnValue().Set(Nan::New<Number>(r->w));
}
NAN_PROPERTY_GETTER(js_rect_hget) {
    qrect *r = (qrect*)Nan::GetInternalFieldPointer(info.This(), 0);
    // printf("qrect retrieving h:%f\n", r->h);
    info.GetReturnValue().Set(Nan::New<Number>(r->h));
}

void js_weak_qtree(const Nan::WeakCallbackInfo<qtree> &data) {
    // printf("Freeing qtree... ");
    qtreed(data.GetParameter());
    // printf("done.\n");
}

NAN_METHOD(js_qtreen) {
    if(!info.IsConstructCall()) {
        Nan::ThrowTypeError("Use the new operator to create instances of a QuadTree.");
    }
    if(info.Length() < 2) {
        Nan::ThrowTypeError("Need a width and height for the QuadTree!");
    }

    int w = info[0]->Int32Value();
    int h = info[1]->Int32Value();
    int ml = info[2]->IsUndefined() ? DEFAULT_MAX_LAYERS : info[2]->Int32Value();
    qtree *qt = qtreen(w, h, ml);
    Nan::SetInternalFieldPointer(info.This(), 0, qt);
    Nan::Persistent<Object> ret(info.This());
    ret.SetWeak(qt, js_weak_qtree, WeakCallbackType::kParameter);
    // ret.MarkIndependent();
    // printf("qt addr new: 0x%lx\n", (unsigned long)qt);
    info.GetReturnValue().Set(info.This());
}

void js_weak_held_persistent(const Nan::WeakCallbackInfo<int> &data) {
}

NAN_METHOD(js_qtreeclear) {
    qtree *qt = (qtree*)Nan::GetInternalFieldPointer(info.This(), 0);
    printf("clear1\n");
    int w = qt->ns[0].r.w;
    printf("clear2\n");
    int h = qt->ns[0].r.h;
    printf("clear3\n");
    int ml = qt->ml;
    printf("clear4\n");
    qtreed(qt);
    printf("clear5\n");
    qt = qtreen(w, h, ml);
    printf("clear6\n");
    Nan::SetInternalFieldPointer(info.This(), 0, qt);
    printf("clear7\n");
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(js_qtree_add) {
    if(info.Length() < 2) {
        Nan::ThrowTypeError("Need a rect and object to add to the QuadTree!");
    }

    qtree *qt = (qtree*)Nan::GetInternalFieldPointer(info.This(), 0);
    // printf("qt addr add: 0x%lx\n", (unsigned long)qt);
    Local<Object> js_rect = info[0]->ToObject();
    Local<Object> d = info[1]->ToObject();
    // printf("Got js_rect object.\n");
    float x = (float)Nan::Get(js_rect, Nan::New<String>("x").ToLocalChecked()).ToLocalChecked()->NumberValue();
    // printf("Got x:%f\n", x);
    float y = (float)Nan::Get(js_rect, Nan::New<String>("y").ToLocalChecked()).ToLocalChecked()->NumberValue();
    // printf("Got y: %f\n", y);
    float w = (float)Nan::Get(js_rect, Nan::New<String>("w").ToLocalChecked()).ToLocalChecked()->NumberValue();
    // printf("Got w: %f\n", w);
    float h = (float)Nan::Get(js_rect, Nan::New<String>("h").ToLocalChecked()).ToLocalChecked()->NumberValue();
    // printf("Got h: %f\n", h);
    // printf("qt add Rect{x:%f,y:%f,w:%f,h:%f}\n", x, y, w, h);
    qrect r = {x, y, w, h};
    // printf("Doing qt add!\n");
    Nan::Persistent<Object> pd(d);
    int garbage = 0;
    pd.SetWeak(&garbage, js_weak_held_persistent, WeakCallbackType::kParameter);
    // pd.MarkIndependent();
    qtreeadd(qt, &r, &pd);
    // printf("Finished add! Returning this.\n");
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(js_qtree_coll) {
    if(info.Length() < 4) {
        Nan::ThrowTypeError("Need x, y, w, h to find collisions with!");
    }

    qtree *qt = (qtree*)Nan::GetInternalFieldPointer(info.This(), 0);
    float x = info[0]->NumberValue();
    float y = info[1]->NumberValue();
    float w = info[2]->NumberValue();
    float h = info[3]->NumberValue();
    qrect r = {x, y, w, h};
    qcolresults *results = qtree_colliding(qt, &r);
    // printf("num results: %d\n", results->ce);
    for(unsigned i = 0; i < results->ce; i++) {
        rect_printfn(&results->e[i].r);
    }
    // Local<Array> ret = Array::New(results->ce);
    Local<Array> ret = Nan::New<Array>(results->ce);
    // if(results->ce > 1000) {
    //     printf("!!!!! %d results!\n", results->ce);
    // }
    for(unsigned i = 0; i < results->ce; i++) {
        Local<Number> x = Nan::New<Number>(results->e[i].r.x);
        Local<Number> y = Nan::New<Number>(results->e[i].r.y);
        Local<Number> w = Nan::New<Number>(results->e[i].r.w);
        Local<Number> h = Nan::New<Number>(results->e[i].r.h);
        Handle<Value> argv[] = {x, y, w, h};
        Local<FunctionTemplate> tplt = Nan::New<FunctionTemplate>(rect_wrap);
        Local<Value> rect = tplt->GetFunction()->NewInstance(4, argv);
        Local<Object> pair = Nan::New<Object>();
        Nan::Set(pair, Nan::New<String>("rect").ToLocalChecked(), rect);
        Nan::Set(pair, Nan::New<String>("obj").ToLocalChecked(), Nan::New<Object>(results->e[i].d));
        Nan::Set(ret, i, pair);
    }
    qcolresultsd(results);
    info.GetReturnValue().Set(ret);
}

NAN_METHOD(js_qtree_remove) {
    if(info.Length() < 1) {
        Nan::ThrowTypeError("Need an object to remove from the QuadTree!");
    }

    qtree *qt = (qtree*)Nan::GetInternalFieldPointer(info.This(), 0);
    Local<Value> d = info[0];
    float x = info[1]->IsUndefined() ? qt->ns->r.x : info[1]->NumberValue();
    float y = info[2]->IsUndefined() ? qt->ns->r.y : info[2]->NumberValue();
    float w = info[3]->IsUndefined() ? qt->ns->r.w : info[3]->NumberValue();
    float h = info[4]->IsUndefined() ? qt->ns->r.h : info[4]->NumberValue();
    qrect r = {x, y, w, h};
    qtreerem(qt, &r, d);
    info.GetReturnValue().Set(info.This());
}

NAN_MODULE_INIT(init) {
    Local<FunctionTemplate> qtree_templ = Nan::New<FunctionTemplate>(js_qtreen);
    qtree_templ->InstanceTemplate()->SetInternalFieldCount(1);
    qtree_templ->SetClassName(Nan::New<String>("native_qtree").ToLocalChecked());
    Nan::SetPrototypeMethod(qtree_templ, "add", js_qtree_add);
    Nan::SetPrototypeMethod(qtree_templ, "intersecting", js_qtree_coll);
    Nan::SetPrototypeMethod(qtree_templ, "remove", js_qtree_remove);
    // Nan::SetPrototypeMethod(qtree_templ, "clear", js_qtreeclear);
    target->Set(Nan::New<String>("QuadTree").ToLocalChecked(), qtree_templ->GetFunction());
    qtree_wrap.Reset(qtree_templ);

    Local<FunctionTemplate> rect_templ = Nan::New<FunctionTemplate>(js_rectn);
    rect_templ->InstanceTemplate()->SetInternalFieldCount(1);
    rect_templ->SetClassName(Nan::New<String>("native_qrect").ToLocalChecked());
    Nan::SetAccessor(rect_templ->InstanceTemplate(), Nan::New<String>("x").ToLocalChecked(), js_rect_xget);
    Nan::SetAccessor(rect_templ->InstanceTemplate(), Nan::New<String>("y").ToLocalChecked(), js_rect_yget);
    Nan::SetAccessor(rect_templ->InstanceTemplate(), Nan::New<String>("w").ToLocalChecked(), js_rect_wget);
    Nan::SetAccessor(rect_templ->InstanceTemplate(), Nan::New<String>("h").ToLocalChecked(), js_rect_hget);
    target->Set(Nan::New<String>("Rect").ToLocalChecked(), rect_templ->GetFunction());
    rect_wrap.Reset(rect_templ);
}

NODE_MODULE(node_quadtree, init)