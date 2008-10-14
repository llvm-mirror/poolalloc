
class SplayRangeSet {
  struct tree_node {
    tree_node* left;
    tree_node* right;
    void* start;
    void* end;
  };

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

 public:

  SplayRangeSet() : Tree(0) {}
  ~SplayRangeSet() { clear(); }
  
  bool insert(void* start, void* end) {
    Tree = splay(Tree, start);
    //If the key is already in, fail the insert
    if (Tree && !key_lt(start, Tree) && !key_gt(start, Tree))
      return false;
    
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
  }

  bool remove(void* key) {
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

  unsigned count() {
    return count_internal(Tree);
  }

  void clear() {
    while (Tree)
      remove (Tree->start);
  }

  bool find(void* key, void*& start, void*& end) {
    if (!Tree) return false;
    Tree = splay(Tree, key);
    if (!key_lt(key, Tree) && !key_gt(key, Tree)) {
      start = Tree->start;
      end = Tree->end;
      return true;
    }
    return false;
  }
  bool find(void* key) {
    if (!Tree) return false;
    Tree = splay(Tree, key);
    if (!key_lt(key, Tree) && !key_gt(key, Tree)) {
      return true;
    }
    return false;
  }

};
