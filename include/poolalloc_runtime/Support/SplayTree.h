template<typename dataTy>
struct range_tree_node {
  range_tree_node* left;
  range_tree_node* right;
  void* start;
  void* end;
  dataTy data;
};

template<>
struct range_tree_node <void>{
  range_tree_node* left;
  range_tree_node* right;
  void* start;
  void* end;
};

template<typename tree_node>
class RangeSplayTree {
  tree_node* Tree;

  tree_node* rotate_right(tree_node* p) {
    tree_node* x = p->left;
    p->left = x->right;
    x->right = p;
    return x;
  }

  tree_node* rotate_left(tree_node* p) {
    tree_node* x = p->right;
    p->right = x->left;
    x->left = p;
    return x;
  }

  bool key_lt(void* _key, tree_node* _t) { return _key < _t->start; };

  bool key_gt(void* _key, tree_node* _t) { return _key > _t->end; };

  /* This function by D. Sleator <sleator@cs.cmu.edu> */
  tree_node* splay (tree_node * t, void* key) {
    tree_node N, *l, *r, *y;
    if (t == 0) return t;
    N.left = N.right = 0;
    l = r = &N;
    
    while(1) {
      if (key_lt(key, t)) {
        if (t->left == 0) break;
        if (key_lt(key, t->left)) {
          y = t->left;                           /* rotate right */
          t->left = y->right;
          y->right = t;
          t = y;
          if (t->left == 0) break;
        }
        r->left = t;                               /* link right */
        r = t;
        t = t->left;
      } else if (key_gt(key, t)) {
        if (t->right == 0) break;
        if (key_gt(key, t->right)) {
          y = t->right;                          /* rotate left */
          t->right = y->left;
          y->left = t;
          t = y;
          if (t->right == 0) break;
        }
        l->right = t;                              /* link left */
        l = t;
        t = t->right;
      } else {
        break;
      }
    }
    l->right = t->left;                                /* assemble */
    r->left = t->right;
    t->left = N.right;
    t->right = N.left;
    return t;
  }

  unsigned count_internal(tree_node* t) {
    if (t)
      return 1 + count_internal(t->left) + count_internal(t->right);
    return 0;
  }

  void __clear_internal(tree_node* t) {
    if (!t) return;
    __clear_internal(t->left);
    __clear_internal(t->right);
    delete t;
  }

 protected:

  RangeSplayTree() : Tree(0) {}
  ~RangeSplayTree() { __clear(); }
  
  tree_node* __insert(void* start, void* end) {
    Tree = splay(Tree, start);
    //If the key is already in, fail the insert
    if (Tree && !key_lt(start, Tree) && !key_gt(start, Tree))
      return 0;
    
    tree_node* n = new tree_node();
    n->start = start;
    n->end = end;
    n->right = n->left = 0;
    if (Tree) {
      if (key_lt(start, Tree)) {
        n->left = Tree->left;
        n->right = Tree;
        Tree->left = 0;
      } else {
        n->right = Tree->right;
        n->left = Tree;
        Tree->right = 0;
      }
    }
    Tree = n;
    return Tree;
  }

  bool __remove(void* key) {
    if (!Tree) return false;
    Tree = splay(Tree, key);
    if (!key_lt(key, Tree) && !key_gt(key, Tree)) {
      tree_node* x = 0;
      if (!Tree->left)
        x = Tree->right;
      else {
        x = splay(Tree->left, key);
        x->right = Tree->right;
      }
      tree_node* y = Tree;
      Tree = x;
      delete y;
      return true;
    }
    return false; /* not there */
  }

  unsigned __count() {
    return count_internal(Tree);
  }

  void __clear() {
    __clear_internal(Tree);
    Tree = 0;
  }

  tree_node* __find(void* key) {
    if (!Tree) return false;
    Tree = splay(Tree, key);
    if (!key_lt(key, Tree) && !key_gt(key, Tree)) {
      return Tree;
    }
    return 0;
  }
};

class RangeSplaySet : RangeSplayTree<range_tree_node<void> > {
 public:
  RangeSplaySet() : RangeSplayTree<range_tree_node<void> >() {}
 
    bool insert(void* start, void* end) {
      return 0 != __insert(start,end);
    }

    bool remove(void* key) {
      return __remove(key);
    }

    bool count() { return __count(); }

    void clear() { __clear(); }

    bool find(void* key, void*& start, void*& end) {
      range_tree_node<void>* t = __find(key);
      if (!t) return false;
      start = t->start;
      end = t->end;
      return true;
    }
    bool find(void* key) {
      range_tree_node<void>* t = __find(key);
      if (!t) return false;
      return true;
    }
};

template<typename T>
class RangeSplayMap : RangeSplayTree<range_tree_node<T> > {
 public:
  RangeSplayMap() : RangeSplayTree<range_tree_node<T> >() {}
 
    using RangeSplayTree<range_tree_node<T> >::__insert;
    using RangeSplayTree<range_tree_node<T> >::__remove;
    using RangeSplayTree<range_tree_node<T> >::__count;
    using RangeSplayTree<range_tree_node<T> >::__clear;
    using RangeSplayTree<range_tree_node<T> >::__find;


    bool insert(void* start, void* end, T& d) {
      range_tree_node<T>* t =  __insert(start,end);
      if (t == 0) return false;
      t->data = d;
      return true;
    }

    bool remove(void* key) {
      return __remove(key);
    }

    bool count() { return __count(); }

    void clear() { __clear(); }

    bool find(void* key, void*& start, void*& end, T& d) {
      range_tree_node<T>* t = __find(key);
      if (!t) return false;
      start = t->start;
      end = t->end;
      d = t->data;
      return true;
    }
    bool find(void* key) {
      range_tree_node<T>* t = __find(key);
      if (!t) return false;
      return true;
    }
};
