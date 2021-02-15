
static DdNode *
MyCuddUniqueInter(
  DdManager * unique,
  int  index,
  DdNode * T,
  DdNode * E);

DD_INLINE
static void
ddFixLimits(
  DdManager *unique)
{
    unique->minDead = (unsigned) (unique->gcFrac * (double) unique->slots);
    unique->cacheSlack = (int) ddMin(unique->maxCacheHard,
	DD_MAX_CACHE_TO_SLOTS_RATIO * unique->slots) -
	2 * (int) unique->cacheSlots;
    if (unique->cacheSlots < unique->slots/2 && unique->cacheSlack >= 0)
	cuddCacheResize(unique);
    return;

} /* end of ddFixLimits */

static int
ddResizeTable(
  DdManager * unique,
  int index)
{
    DdSubtable *newsubtables;
    DdNodePtr *newnodelist;
    DdNodePtr *newvars;
    DdNode *sentinel = &(unique->sentinel);
    int oldsize,newsize;
    int i,j,reorderSave;
    int numSlots = unique->initSlots;
    int *newperm, *newinvperm, *newmap;
    DdNode *one, *zero;

    oldsize = unique->size;
    /* Easy case: there is still room in the current table. */
    if (index < unique->maxSize) {
	for (i = oldsize; i <= index; i++) {
	    unique->subtables[i].slots = numSlots;
	    unique->subtables[i].shift = sizeof(int) * 8 -
		cuddComputeFloorLog2(numSlots);
	    unique->subtables[i].keys = 0;
	    unique->subtables[i].maxKeys = numSlots * DD_MAX_SUBTABLE_DENSITY;
	    unique->subtables[i].dead = 0;
	    unique->perm[i] = i;
	    unique->invperm[i] = i;
	    newnodelist = unique->subtables[i].nodelist =
		ALLOC(DdNodePtr, numSlots);
	    if (newnodelist == NULL) {
		unique->errorCode = CUDD_MEMORY_OUT;
		return(0);
	    }
	    for (j = 0; j < numSlots; j++) {
		newnodelist[j] = sentinel;
	    }
	}
	if (unique->map != NULL) {
	    for (i = oldsize; i <= index; i++) {
		unique->map[i] = i;
	    }
	}
    } else {
	/* The current table is too small: we need to allocate a new,
	** larger one; move all old subtables, and initialize the new
	** subtables up to index included.
	*/
	newsize = index + DD_DEFAULT_RESIZE;
#ifdef DD_VERBOSE
	(void) fprintf(unique->err,
		       "Increasing the table size from %d to %d\n",
		       unique->maxSize, newsize);
#endif
	newsubtables = ALLOC(DdSubtable,newsize);
	if (newsubtables == NULL) {
	    unique->errorCode = CUDD_MEMORY_OUT;
	    return(0);
	}
	newvars = ALLOC(DdNodePtr,newsize);
	if (newvars == NULL) {
	    unique->errorCode = CUDD_MEMORY_OUT;
	    return(0);
	}
	newperm = ALLOC(int,newsize);
	if (newperm == NULL) {
	    unique->errorCode = CUDD_MEMORY_OUT;
	    return(0);
	}
	newinvperm = ALLOC(int,newsize);
	if (newinvperm == NULL) {
	    unique->errorCode = CUDD_MEMORY_OUT;
	    return(0);
	}
	if (unique->map != NULL) {
	    newmap = ALLOC(int,newsize);
	    if (newmap == NULL) {
		unique->errorCode = CUDD_MEMORY_OUT;
		return(0);
	    }
	    unique->memused += (newsize - unique->maxSize) * sizeof(int);
	}
	unique->memused += (newsize - unique->maxSize) * ((numSlots+1) *
	    sizeof(DdNode *) + 2 * sizeof(int) + sizeof(DdSubtable));
	if (newsize > unique->maxSizeZ) {
	    FREE(unique->stack);
	    unique->stack = ALLOC(DdNodePtr,newsize + 1);
	    if (unique->stack == NULL) {
		unique->errorCode = CUDD_MEMORY_OUT;
		return(0);
	    }
	    unique->stack[0] = NULL; /* to suppress harmless UMR */
	    unique->memused +=
		(newsize - ddMax(unique->maxSize,unique->maxSizeZ))
		* sizeof(DdNode *);
	}
	for (i = 0; i < oldsize; i++) {
	    newsubtables[i].slots = unique->subtables[i].slots;
	    newsubtables[i].shift = unique->subtables[i].shift;
	    newsubtables[i].keys = unique->subtables[i].keys;
	    newsubtables[i].maxKeys = unique->subtables[i].maxKeys;
	    newsubtables[i].dead = unique->subtables[i].dead;
	    newsubtables[i].nodelist = unique->subtables[i].nodelist;
	    newvars[i] = unique->vars[i];
	    newperm[i] = unique->perm[i];
	    newinvperm[i] = unique->invperm[i];
	}
	for (i = oldsize; i <= index; i++) {
	    newsubtables[i].slots = numSlots;
	    newsubtables[i].shift = sizeof(int) * 8 -
		cuddComputeFloorLog2(numSlots);
	    newsubtables[i].keys = 0;
	    newsubtables[i].maxKeys = numSlots * DD_MAX_SUBTABLE_DENSITY;
	    newsubtables[i].dead = 0;
	    newperm[i] = i;
	    newinvperm[i] = i;
	    newnodelist = newsubtables[i].nodelist = ALLOC(DdNodePtr, numSlots);
	    if (newnodelist == NULL) {
		unique->errorCode = CUDD_MEMORY_OUT;
		return(0);
	    }
	    for (j = 0; j < numSlots; j++) {
		newnodelist[j] = sentinel;
	    }
	}
	if (unique->map != NULL) {
	    for (i = 0; i < oldsize; i++) {
		newmap[i] = unique->map[i];
	    }
	    for (i = oldsize; i <= index; i++) {
		newmap[i] = i;
	    }
	    FREE(unique->map);
	    unique->map = newmap;
	}
	FREE(unique->subtables);
	unique->subtables = newsubtables;
	unique->maxSize = newsize;
	FREE(unique->vars);
	unique->vars = newvars;
	FREE(unique->perm);
	unique->perm = newperm;
	FREE(unique->invperm);
	unique->invperm = newinvperm;
    }
    unique->slots += (index + 1 - unique->size) * numSlots;
    ddFixLimits(unique);
    unique->size = index + 1;

    /* Now that the table is in a coherent state, create the new
    ** projection functions. We need to temporarily disable reordering,
    ** because we cannot reorder without projection functions in place.
    **/
    one = unique->one;
    zero = Cudd_Not(one);

    reorderSave = unique->autoDyn;
    unique->autoDyn = 0;
    for (i = oldsize; i <= index; i++) {
	unique->vars[i] = MyCuddUniqueInter(unique,i,one,zero);
	if (unique->vars[i] == NULL) {
	    unique->autoDyn = reorderSave;
	    return(0);
	}
	cuddRef(unique->vars[i]);
    }
    unique->autoDyn = reorderSave;

    return(1);

}

static DdNode *
MyCuddUniqueInter(
  DdManager * unique,
  int  index,
  DdNode * T,
  DdNode * E)
{
    int pos;
    unsigned int level;
    int retval;
    DdNodePtr *nodelist;
    DdNode *looking;
    DdNodePtr *previousP;
    DdSubtable *subtable;
    int gcNumber;

#ifdef DD_UNIQUE_PROFILE
    unique->uniqueLookUps++;
#endif

    if (index >= unique->size) {
	if (!ddResizeTable(unique,index)) return(NULL);
    }

    level = unique->perm[index];
    subtable = &(unique->subtables[level]);

#ifdef DD_DEBUG
    assert(level < (unsigned) cuddI(unique,T->index));
    assert(level < (unsigned) cuddI(unique,Cudd_Regular(E)->index));
#endif

    pos = ddHash(T, E, subtable->shift);
    nodelist = subtable->nodelist;
    previousP = &(nodelist[pos]);
    looking = *previousP;

    while (T < cuddT(looking)) {
	previousP = &(looking->next);
	looking = *previousP;
#ifdef DD_UNIQUE_PROFILE
	unique->uniqueLinks++;
#endif
    }
    while (T == cuddT(looking) && E < cuddE(looking)) {
	previousP = &(looking->next);
	looking = *previousP;
#ifdef DD_UNIQUE_PROFILE
	unique->uniqueLinks++;
#endif
    }
    if (T == cuddT(looking) && E == cuddE(looking)) {
	if (looking->ref == 0) {
	    cuddReclaim(unique,looking);
	}
	return(looking);
    }

    /* countDead is 0 if deads should be counted and ~0 if they should not. */
    if (unique->autoDyn &&
    unique->keys - (unique->dead & unique->countDead) >= unique->nextDyn) {
#ifdef DD_DEBUG
	retval = Cudd_DebugCheck(unique);
	if (retval != 0) return(NULL);
	retval = Cudd_CheckKeys(unique);
	if (retval != 0) return(NULL);
#endif
	retval = Cudd_ReduceHeap(unique,unique->autoMethod,10); /* 10 = whatever */
	if (retval == 0) unique->reordered = 2;
#ifdef DD_DEBUG
	retval = Cudd_DebugCheck(unique);
	if (retval != 0) unique->reordered = 2;
	retval = Cudd_CheckKeys(unique);
	if (retval != 0) unique->reordered = 2;
#endif
	return(NULL);
    }

    if (subtable->keys > subtable->maxKeys) {
        if (unique->gcEnabled &&
	    ((unique->dead > unique->minDead) ||
	    ((unique->dead > unique->minDead / 2) &&
	    (subtable->dead > subtable->keys * 0.95)))) { /* too many dead */
	    (void) cuddGarbageCollect(unique,1);
	} else {
	    cuddRehash(unique,(int)level);
	}
	/* Update pointer to insertion point. In the case of rehashing,
	** the slot may have changed. In the case of garbage collection,
	** the predecessor may have been dead. */
	pos = ddHash(T, E, subtable->shift);
	nodelist = subtable->nodelist;
	previousP = &(nodelist[pos]);
	looking = *previousP;

	while (T < cuddT(looking)) {
	    previousP = &(looking->next);
	    looking = *previousP;
#ifdef DD_UNIQUE_PROFILE
	    unique->uniqueLinks++;
#endif
	}
	while (T == cuddT(looking) && E < cuddE(looking)) {
	    previousP = &(looking->next);
	    looking = *previousP;
#ifdef DD_UNIQUE_PROFILE
	    unique->uniqueLinks++;
#endif
	}
    }

    unique->keys++;
    subtable->keys++;

    /* Ref T & E before calling cuddAllocNode, because it may garbage
    ** collect, and then T & E would be lost.
    */
    cuddSatInc(T->ref);		/* we know T is a regular pointer */
    cuddRef(E);
    gcNumber = unique->garbageCollections;
    looking = cuddAllocNode(unique);
    if (looking == NULL) return(NULL);
    if (gcNumber != unique->garbageCollections) {
	DdNode *looking2;
	pos = ddHash(T, E, subtable->shift);
	nodelist = subtable->nodelist;
	previousP = &(nodelist[pos]);
	looking2 = *previousP;

	while (T < cuddT(looking2)) {
	    previousP = &(looking2->next);
	    looking2 = *previousP;
#ifdef DD_UNIQUE_PROFILE
	    unique->uniqueLinks++;
#endif
	}
	while (T == cuddT(looking2) && E < cuddE(looking2)) {
	    previousP = &(looking2->next);
	    looking2 = *previousP;
#ifdef DD_UNIQUE_PROFILE
	    unique->uniqueLinks++;
#endif
	}
    }
    looking->ref = 0;
    looking->index = index;
    cuddT(looking) = T;
    cuddE(looking) = E;
    looking->next = *previousP;
    *previousP = looking;

#ifdef DD_DEBUG
    cuddCheckCollisionOrdering(unique,level,pos);
#endif

    return(looking);

} /* end of MyCuddUniqueInter */

static void
MyCuddCacheInsert(
  DdManager * table,
  ptruint op,
  DdNode * f,
  DdNode * g,
  DdNode * h,
  DdNode * data)
{
    int posn;
    register DdCache *entry;
    ptruint uf, ug, uh;

    uf = (ptruint) f | (op & 0xe);
    ug = (ptruint) g | (op >> 4);
    uh = (ptruint) h;

    posn = ddCHash2(uh,uf,ug,table->cacheShift);
    entry = &table->cache[posn];

    table->cachecollisions += entry->data != NULL;
    table->cacheinserts++;

    entry->f = (DdNode *) uf;
    entry->g = (DdNode *) ug;
    entry->h = (ptruint) h;
    entry->data = data;
#ifdef DD_CACHE_PROFILE
    entry->count++;
#endif

} /* end of MyCuddCacheInsert */

static void
MyCuddCacheInsert2(
  DdManager * table,
  DdNode * (*op)(DdManager *, DdNode *, DdNode *),
  DdNode * f,
  DdNode * g,
  DdNode * data)
{
    int posn;
    register DdCache *entry;

    posn = ddCHash2(op,f,g,table->cacheShift);
    entry = &table->cache[posn];

    if (entry->data != NULL) {
        table->cachecollisions++;
    }
    table->cacheinserts++;

    entry->f = f;
    entry->g = g;
    entry->h = (ptruint) op;
    entry->data = data;
#ifdef DD_CACHE_PROFILE
    entry->count++;
#endif

} /* end of MyCuddCacheInsert2 */

static DdNode *
MyCuddCacheLookup(
  DdManager * table,
  ptruint op,
  DdNode * f,
  DdNode * g,
  DdNode * h)
{
    int posn;
    DdCache *en,*cache;
    DdNode *data;
    ptruint uf, ug, uh;

    uf = (ptruint) f | (op & 0xe);
    ug = (ptruint) g | (op >> 4);
    uh = (ptruint) h;

    cache = table->cache;
#ifdef DD_DEBUG
    if (cache == NULL) {
        return(NULL);
    }
#endif

    posn = ddCHash2(uh,uf,ug,table->cacheShift);
    en = &cache[posn];
    if (en->data != NULL && en->f==(DdNodePtr)uf && en->g==(DdNodePtr)ug &&
        en->h==uh) {
        data = Cudd_Regular(en->data);
        table->cacheHits++;
        if (data->ref == 0) {
            cuddReclaim(table,data);
        }
        return(en->data);
    }

    /* Cache miss: decide whether to resize. */
    table->cacheMisses++;

    if (table->cacheSlack >= 0 &&
        table->cacheHits > table->cacheMisses * table->minHit) {
        cuddCacheResize(table);
    }

    return(NULL);

} /* end of MyCuddCacheLookup */

static DdNode *
MyCuddCacheLookup2(
  DdManager * table,
  DdNode * (*op)(DdManager *, DdNode *, DdNode *),
  DdNode * f,
  DdNode * g)
{
    int posn;
    DdCache *en,*cache;
    DdNode *data;

    cache = table->cache;
#ifdef DD_DEBUG
    if (cache == NULL) {
        return(NULL);
    }
#endif

    posn = ddCHash2(op,f,g,table->cacheShift);
    en = &cache[posn];
    if (en->data != NULL && en->f==f && en->g==g && en->h==(ptruint)op) {
        data = Cudd_Regular(en->data);
        table->cacheHits++;
        if (data->ref == 0) {
            cuddReclaim(table,data);
        }
        return(en->data);
    }

    /* Cache miss: decide whether to resize. */
    table->cacheMisses++;

    if (table->cacheSlack >= 0 &&
        table->cacheHits > table->cacheMisses * table->minHit) {
        cuddCacheResize(table);
    }

    return(NULL);

} /* end of cuddCacheLookup2 */

static DdNode *
MyCuddBddAndRecur(
  DdManager * manager,
  DdNode * f,
  DdNode * g)
{
    DdNode *F, *fv, *fnv, *G, *gv, *gnv;
    DdNode *one, *zero, *r, *t, *e;
    unsigned int topf, topg, index, top;

    MyCuddBddAndRecurDebug(manager,f,g);

    statLine(manager);
    one = DD_ONE(manager);
    zero = Cudd_Not(one);

    /* Terminal cases. */
    if (f == zero || g == zero || f == Cudd_Not(g)){
        return(zero);
    };
    if (f == g || g == one){
        return(f);
    };
    if (f == one){
        return(g);
    };

    /* At this point f and g are not constant. */
    if (f > g) { // Try to increase cache efficiency.
        DdNode *tmp = f;
        f = g;
        g = tmp;
    }

    /* Check cache. */
    F = Cudd_Regular(f);
    G = Cudd_Regular(g);

    #if 1
    if (F->ref != 1 || G->ref != 1) {
        r = MyCuddCacheLookup2(manager, Cudd_bddAnd, f, g);
        if (r != NULL) return(r);
    }
    #endif

    /* Here we can skip the use of cuddI, because the operands are known
    ** to be non-constant.
    */
    topf = manager->perm[F->index];
    topg = manager->perm[G->index];

    /* Compute cofactors. */
    if (topf <= topg) {
        index = F->index;
        fv = cuddT(F);
        fnv = cuddE(F);
        if (Cudd_IsComplement(f)) {
            fv = Cudd_Not(fv);
            fnv = Cudd_Not(fnv);
        }
    } else {
        index = G->index;
        fv = fnv = f;
    }

    if (topg <= topf) {
        gv = cuddT(G);
        gnv = cuddE(G);
        if (Cudd_IsComplement(g)) {
            gv = Cudd_Not(gv);
            gnv = Cudd_Not(gnv);
        }
    } else {
        gv = gnv = g;
    }

    t = MyCuddBddAndRecur(manager, fv, gv);
    if (t == NULL) return(NULL);
    cuddRef(t);

    e = MyCuddBddAndRecur(manager, fnv, gnv);

    if (e == NULL) {
        Cudd_IterDerefBdd(manager, t);
        return(NULL);
    }
    cuddRef(e);

    top = ddMin(topf,topg);

    if (t == e) {
        r = t;
    } else {
        if (Cudd_IsComplement(t)) {
            r = MyCuddUniqueInter(manager,(int)index,Cudd_Not(t),Cudd_Not(e));
            if (r == NULL) {
            Cudd_IterDerefBdd(manager, t);
            Cudd_IterDerefBdd(manager, e);
            return(NULL);
            }
            r = Cudd_Not(r);
        } else {
            r = MyCuddUniqueInter(manager,(int)index,t,e);
            if (r == NULL) {
            Cudd_IterDerefBdd(manager, t);
            Cudd_IterDerefBdd(manager, e);
            return(NULL);
            }
        }
    }
    cuddDeref(e);
    cuddDeref(t);
    #if 1
    if (F->ref != 1 || G->ref != 1)
        MyCuddCacheInsert2(manager, Cudd_bddAnd, f, g, r);
    #endif
    return(r);
}

static int
MyBddVarToCanonicalSimple(
  DdManager * dd,
  DdNode ** fp,
  DdNode ** gp,
  DdNode ** hp,
  unsigned int * topfp,
  unsigned int * topgp,
  unsigned int * tophp)
{
    register DdNode		*F, *G, *r, *f, *g, *h;
    register unsigned int	topf, topg;
    int				comple, change;

    f = *fp;
    g = *gp;
    h = *hp;

    change = 0;

    if (g == Cudd_Not(h)) {	/* ITE(F,G,!G) = ITE(G,F,!F) */
        F = Cudd_Regular(f);
        topf = cuddI(dd,F->index);
        G = Cudd_Regular(g);
        topg = cuddI(dd,G->index);
        if ((topf > topg) || (topf == topg && f > g)) {
            r = f;
            f = g;
            g = r;
            h = Cudd_Not(r);
            change = 1;
        }
    }
    /* adjust pointers so that the first 2 arguments to ITE are regular */
    if (Cudd_IsComplement(f)) {	/* ITE(!F,G,H) = ITE(F,H,G) */
        f = Cudd_Not(f);
        r = g;
        g = h;
        h = r;
        change = 1;
    }
    comple = 0;
    if (Cudd_IsComplement(g)) {	/* ITE(F,!G,H) = !ITE(F,G,!H) */
        g = Cudd_Not(g);
        h = Cudd_Not(h);
        change = 1;
        comple = 1;
    }
    if (change) {
        *fp = f;
        *gp = g;
        *hp = h;
    }

    /* Here we can skip the use of cuddI, because the operands are known
    ** to be non-constant.
    */
    *topfp = dd->perm[f->index];
    *topgp = dd->perm[g->index];
    *tophp = dd->perm[Cudd_Regular(h)->index];

    return(comple);

} /* end of bddVarToCanonicalSimple */

static DdNode *
MyCuddBddIteRecur(
  DdManager * dd,
  DdNode * f,
  DdNode * g,
  DdNode * h)
{
    DdNode	 *one, *zero, *res;
    DdNode	 *r, *Fv, *Fnv, *Gv, *Gnv, *H, *Hv, *Hnv, *t, *e;
    unsigned int topf, topg, toph, v;
    int		 index;
    int		 comple;

    statLine(dd);
    /* Terminal cases. */

    /* One variable cases. */
    if (f == (one = DD_ONE(dd))) 	/* ITE(1,G,H) = G */
	return(g);

    if (f == (zero = Cudd_Not(one))) 	/* ITE(0,G,H) = H */
	return(h);

    /* From now on, f is known not to be a constant. */
    if (g == one || f == g) {	/* ITE(F,F,H) = ITE(F,1,H) = F + H */
	if (h == zero) {	/* ITE(F,1,0) = F */
	    return(f);
	} else {
	    res = MyCuddBddAndRecur(dd,Cudd_Not(f),Cudd_Not(h));
	    return(Cudd_NotCond(res,res != NULL));
	}
    } else if (g == zero || f == Cudd_Not(g)) { /* ITE(F,!F,H) = ITE(F,0,H) = !F * H */
	if (h == one) {		/* ITE(F,0,1) = !F */
	    return(Cudd_Not(f));
	} else {
	    res = MyCuddBddAndRecur(dd,Cudd_Not(f),h);
	    return(res);
	}
    }
    if (h == zero || f == h) {    /* ITE(F,G,F) = ITE(F,G,0) = F * G */
        res = MyCuddBddAndRecur(dd,f,g);
        return(res);
    } else if (h == one || f == Cudd_Not(h)) { /* ITE(F,G,!F) = ITE(F,G,1) = !F + G */
        res = MyCuddBddAndRecur(dd,f,Cudd_Not(g));
        return(Cudd_NotCond(res,res != NULL));
    }

    /* Check remaining one variable case. */
    if (g == h) { 		/* ITE(F,G,G) = G */
        return(g);
    }

    /* From here, there are no constants. */
    comple = MyBddVarToCanonicalSimple(dd, &f, &g, &h, &topf, &topg, &toph);

    /* f & g are now regular pointers */

    v = ddMin(topg, toph);

    /* A shortcut: ITE(F,G,H) = (v,G,H) if F = (v,1,0), v < top(G,H). */
    if (topf < v && cuddT(f) == one && cuddE(f) == zero) {
        r = MyCuddUniqueInter(dd, (int) f->index, g, h);
        return(Cudd_NotCond(r,comple && r != NULL));
    }

    /* Check cache. */
    r = MyCuddCacheLookup(dd, DD_BDD_ITE_TAG, f, g, h);
    if (r != NULL) {
        return(Cudd_NotCond(r,comple));
    }

    /* Compute cofactors. */
    if (topf <= v) {
        v = ddMin(topf, v);	/* v = top_var(F,G,H) */
        index = f->index;
        Fv = cuddT(f); Fnv = cuddE(f);
    } else {
        Fv = Fnv = f;
    }
    if (topg == v) {
        index = g->index;
        Gv = cuddT(g); Gnv = cuddE(g);
    } else {
        Gv = Gnv = g;
    }
    if (toph == v) {
        H = Cudd_Regular(h);
        index = H->index;
        Hv = cuddT(H); Hnv = cuddE(H);
        if (Cudd_IsComplement(h)) {
            Hv = Cudd_Not(Hv);
            Hnv = Cudd_Not(Hnv);
        }
    } else {
        Hv = Hnv = h;
    }

    /* Recursive step. */
    t = MyCuddBddIteRecur(dd,Fv,Gv,Hv);
    if (t == NULL) return(NULL);
    cuddRef(t);

    e = MyCuddBddIteRecur(dd,Fnv,Gnv,Hnv);
    if (e == NULL) {
        Cudd_IterDerefBdd(dd,t);
        return(NULL);
    }
    cuddRef(e);

    r = (t == e) ? t : MyCuddUniqueInter(dd,index,t,e);
    if (r == NULL) {
        Cudd_IterDerefBdd(dd,t);
        Cudd_IterDerefBdd(dd,e);
        return(NULL);
    }
    cuddDeref(t);
    cuddDeref(e);

    MyCuddCacheInsert(dd, DD_BDD_ITE_TAG, f, g, h, r);
    return(Cudd_NotCond(r,comple));

} /* end of MyCuddBddIteRecur */

static DdNode *
MyCuddBddExistAbstractRecur(
  DdManager * manager,
  DdNode * f,
  DdNode * cube)
{
    DdNode	*F, *T, *E, *res, *res1, *res2, *one;

    statLine(manager);
    one = DD_ONE(manager);
    F = Cudd_Regular(f);

    /* Cube is guaranteed to be a cube at this point. */
    if (cube == one || F == one) {
        return(f);
    }
    /* From now on, f and cube are non-constant. */

    /* Abstract a variable that does not appear in f. */
    while (manager->perm[F->index] > manager->perm[cube->index]) {
        cube = cuddT(cube);
        if (cube == one) return(f);
    }

    /* Check the cache. */
    #if 0
    if (F->ref != 1 && (res = MyCuddCacheLookup2(manager, Cudd_bddExistAbstract, f, cube)) != NULL) {
        return(res);
    }
    #endif

    /* Compute the cofactors of f. */
    T = cuddT(F); E = cuddE(F);
    if (f != F) {
        T = Cudd_Not(T); E = Cudd_Not(E);
    }

    /* If the two indices are the same, so are their levels. */
    if (F->index == cube->index) {
        if (T == one || E == one || T == Cudd_Not(E)) {
            return(one);
        }
        res1 = MyCuddBddExistAbstractRecur(manager, T, cuddT(cube));
        if (res1 == NULL) return(NULL);
        if (res1 == one) {
            #if 0
            if (F->ref != 1)
            MyCuddCacheInsert2(manager, Cudd_bddExistAbstract, f, cube, one);
            #endif
            return(one);
        }
        cuddRef(res1);
        res2 = MyCuddBddExistAbstractRecur(manager, E, cuddT(cube));
        if (res2 == NULL) {
            Cudd_IterDerefBdd(manager,res1);
            return(NULL);
        }
        cuddRef(res2);
        res = MyCuddBddAndRecur(manager, Cudd_Not(res1), Cudd_Not(res2));
        if (res == NULL) {
            Cudd_IterDerefBdd(manager, res1);
            Cudd_IterDerefBdd(manager, res2);
            return(NULL);
        }
        res = Cudd_Not(res);
        cuddRef(res);
        Cudd_IterDerefBdd(manager, res1);
        Cudd_IterDerefBdd(manager, res2);
        #if 0
        if (F->ref != 1)
            MyCuddCacheInsert2(manager, Cudd_bddExistAbstract, f, cube, res);
        #endif
        cuddDeref(res);
        return(res);
    } else { /* if (cuddI(manager,F->index) < cuddI(manager,cube->index)) */
	res1 = MyCuddBddExistAbstractRecur(manager, T, cube);
	if (res1 == NULL) return(NULL);
        cuddRef(res1);
	res2 = MyCuddBddExistAbstractRecur(manager, E, cube);
	if (res2 == NULL) {
	    Cudd_IterDerefBdd(manager, res1);
	    return(NULL);
	}
    cuddRef(res2);
	/* ITE takes care of possible complementation of res1 */
	res = MyCuddBddIteRecur(manager, manager->vars[F->index], res1, res2);
	if (res == NULL) {
	    Cudd_IterDerefBdd(manager, res1);
	    Cudd_IterDerefBdd(manager, res2);
	    return(NULL);
	}
	cuddDeref(res1);
	cuddDeref(res2);

	#if 0
	if (F->ref != 1)
	    MyCuddCacheInsert2(manager, Cudd_bddExistAbstract, f, cube, res);
    #endif
        return(res);
    }
}

typedef struct
{
    DdNode* result;
    char valid;
} TerminalCaseResult;

static TerminalCaseResult MyCuddBddAndAbstractRecurTerminalCase(DdNode* f,DdNode* g,DdNode* zero,DdNode* one)
{
    TerminalCaseResult res = {};
    res.valid = true;

    WriteToFileNTimes("/home/ruben/CleanNuSMV/tests/AndAbstractTerminalCase.out",0,(int) f);
    WriteToFileNTimes("/home/ruben/CleanNuSMV/tests/AndAbstractTerminalCase.out",0,(int) g);
    WriteToFileNTimes("/home/ruben/CleanNuSMV/tests/AndAbstractTerminalCase.out",0,(int) zero);
    WriteToFileNTimes("/home/ruben/CleanNuSMV/tests/AndAbstractTerminalCase.out",0,(int) one);

    if (f == zero || g == zero || f == Cudd_Not(g))
    {
        res.result = zero;

        WriteToFileNTimes("/home/ruben/CleanNuSMV/tests/AndAbstractTerminalCase.out",0,(int) res.result);

        return res;
    }

    if (f == one && g == one)
    {
        res.result = one;

        WriteToFileNTimes("/home/ruben/CleanNuSMV/tests/AndAbstractTerminalCase.out",0,(int) res.result);

        return res;
    }

    res.valid = false;

    WriteToFileNTimes("/home/ruben/CleanNuSMV/tests/AndAbstractTerminalCase.out",0,(int) 0);

    return res;
}

static DdNode *
MyCuddBddAndAbstractRecur(
  DdManager * manager,
  DdNode * f,
  DdNode * g,
  DdNode * cube)
{
    DdNode *F, *ft, *fe, *G, *gt, *ge;
    DdNode *one, *zero, *r, *t, *e;
    unsigned int topf, topg, topcube, top, index;

    statLine(manager);
    one = DD_ONE(manager);
    zero = Cudd_Not(one);

    /* Terminal cases. */
    if (f == zero || g == zero || f == Cudd_Not(g)) return(zero);
    if (f == one && g == one)	return(one);

    if (cube == one) {
        return(MyCuddBddAndRecur(manager, f, g));
    }
    if (f == one || f == g) {
        return(MyCuddBddExistAbstractRecur(manager, g, cube));
    }
    if (g == one) {
        return(MyCuddBddExistAbstractRecur(manager, f, cube));
    }
    /* At this point f, g, and cube are not constant. */

    if (f > g) { /* Try to increase cache efficiency. */
        DdNode *tmp = f;
        f = g;
        g = tmp;
    }

    /* Here we can skip the use of cuddI, because the operands are known
    ** to be non-constant.
    */
    F = Cudd_Regular(f);
    G = Cudd_Regular(g);
    topf = manager->perm[F->index];
    topg = manager->perm[G->index];
    top = ddMin(topf, topg);
    topcube = manager->perm[cube->index];

    while (topcube < top) {
        cube = cuddT(cube);
        if (cube == one) {
            return(MyCuddBddAndRecur(manager, f, g));
        }
        topcube = manager->perm[cube->index];
    }
    /* Now, topcube >= top. */

    /* Check cache. */
    #if 0
    if (F->ref != 1 || G->ref != 1) {
	r = MyCuddCacheLookup(manager, DD_BDD_AND_ABSTRACT_TAG, f, g, cube);
	if (r != NULL) {
	    return(r);
	}
    }
    #endif

    if (topf == top) {
        index = F->index;
        ft = cuddT(F);
        fe = cuddE(F);
        if (Cudd_IsComplement(f)) {
            ft = Cudd_Not(ft);
            fe = Cudd_Not(fe);
        }
    } else {
        index = G->index;
        ft = fe = f;
    }

    if (topg == top) {
	gt = cuddT(G);
	ge = cuddE(G);
	if (Cudd_IsComplement(g)) {
	    gt = Cudd_Not(gt);
	    ge = Cudd_Not(ge);
	}
    } else {
        gt = ge = g;
    }

    if (topcube == top) {	/* quantify */
	DdNode *Cube = cuddT(cube);
	t = MyCuddBddAndAbstractRecur(manager, ft, gt, Cube);
	if (t == NULL) return(NULL);
	/* Special case: 1 OR anything = 1. Hence, no need to compute
	** the else branch if t is 1. Likewise t + t * anything == t.
	** Notice that t == fe implies that fe does not depend on the
	** variables in Cube. Likewise for t == ge.
	*/
	if (t == one || t == fe || t == ge) {
	    if (F->ref != 1 || G->ref != 1)
		MyCuddCacheInsert(manager, DD_BDD_AND_ABSTRACT_TAG,
				f, g, cube, t);
	    return(t);
	}
	cuddRef(t);
	/* Special case: t + !t * anything == t + anything. */
	if (t == Cudd_Not(fe)) {
	    e = MyCuddBddExistAbstractRecur(manager, ge, Cube);
	} else if (t == Cudd_Not(ge)) {
	    e = MyCuddBddExistAbstractRecur(manager, fe, Cube);
	} else {
	    e = MyCuddBddAndAbstractRecur(manager, fe, ge, Cube);
	}
	if (e == NULL) {
	    Cudd_IterDerefBdd(manager, t);
	    return(NULL);
	}
	if (t == e) {
	    r = t;
	    cuddDeref(t);
	} else {
	    cuddRef(e);
	    r = MyCuddBddAndRecur(manager, Cudd_Not(t), Cudd_Not(e));
	    if (r == NULL) {
		Cudd_IterDerefBdd(manager, t);
		Cudd_IterDerefBdd(manager, e);
		return(NULL);
	    }
	    r = Cudd_Not(r);
	    cuddRef(r);
	    Cudd_DelayedDerefBdd(manager, t);
	    Cudd_DelayedDerefBdd(manager, e);
	    cuddDeref(r);
	}
    } else {
	t = MyCuddBddAndAbstractRecur(manager, ft, gt, cube);
	if (t == NULL) return(NULL);
	cuddRef(t);
	e = MyCuddBddAndAbstractRecur(manager, fe, ge, cube);
	if (e == NULL) {
	    Cudd_IterDerefBdd(manager, t);
	    return(NULL);
	}
	if (t == e) {
	    r = t;
	    cuddDeref(t);
	} else {
	    cuddRef(e);
	    if (Cudd_IsComplement(t)) {
		r = MyCuddUniqueInter(manager, (int) index,
				    Cudd_Not(t), Cudd_Not(e));
		if (r == NULL) {
		    Cudd_IterDerefBdd(manager, t);
		    Cudd_IterDerefBdd(manager, e);
		    return(NULL);
		}
		r = Cudd_Not(r);
	    } else {
		r = MyCuddUniqueInter(manager,(int)index,t,e);
		if (r == NULL) {
		    Cudd_IterDerefBdd(manager, t);
		    Cudd_IterDerefBdd(manager, e);
		    return(NULL);
		}
	    }
	    cuddDeref(e);
	    cuddDeref(t);
	}
    }

    #if 0
    if (F->ref != 1 || G->ref != 1)
        MyCuddCacheInsert(manager, DD_BDD_AND_ABSTRACT_TAG, f, g, cube, r);
    #endif

    return (r);
}

