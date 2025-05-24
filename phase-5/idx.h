#ifndef IDX_H
#define IDX_H


typedef struct BTreeNode {
	int numKeys;
	int *keys;
	void **values;
	struct BTreeNode **children;
	int isLeaf;
	struct BTreeNode *next;
	struct BTreeNode *prev;
	struct BTreeNode *parent;
} BTreeNode;

typedef struct BTree {
	BTreeNode *root;
} BTree;

void traverseBTree(BTreeNode *node, int depth) ;
BTreeNode *createBTreeNode(int isLeaf);
void print_btree();
void debugParentPointers(BTreeNode *node, int depth) ;
int findChildIndex(BTreeNode *node, int key);
/*void promote_to_parent(BTreeNode** root, BTreeNode* node, int promotedKey, BTreeNode *promoted);*/
void insert(BTreeNode **root, BTreeNode*node, int key, void *value);
void traverseBTree(BTreeNode *node, int depth);
void get(BTreeNode *node, int key);
void printBTree(BTreeNode *node, int depth);
void destroy(BTreeNode **root, BTreeNode *node, int key);
#endif // BTREE_H
