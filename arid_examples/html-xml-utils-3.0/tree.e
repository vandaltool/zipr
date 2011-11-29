 typedef enum {
  Element, Text, Comment, Declaration, Procins, Root
} Nodetype;
 typedef struct _node {
  Nodetype tp;
  string name;
  pairlist attribs;
  string text;
  string url;
  struct _node *parent;
  struct _node *sister;
  struct _node *children;
} Node, *Tree;
 Tree create(void);
 void tree_delete(Tree t);
 Tree get_root(Tree t);
 Boolean get_attrib(Node *e, const string attname, string *val);
 void set_attrib(Node *e, string name, string value);
 Tree wrap_contents(Node *n, const string elem, pairlist attr);
 Boolean is_known(const string e);
 Boolean is_pre(const string e);
 Boolean need_stag(const string e);
 Boolean need_etag(const string e);
 Boolean is_empty(const string e);
 Boolean is_mixed(const string e);
 Boolean break_before(const string e);
 Boolean break_after(const string e);
 Tree html_push(Tree t, string elem, pairlist attr);
 Tree html_pop(Tree t, string elem);
 Tree append_comment(Tree t, string comment);
 Tree append_declaration(Tree t, string gi,
          string fpi, string url);
 Tree append_procins(Tree t, string procins);
 Tree append_text(Tree t, string text);
 void dumptree(Tree t);
