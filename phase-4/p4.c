#include <stdio.h>
#include <stdlib.h>

#define DEGREE 3


typedef struct BTreeNode {
	int numKeys;
	int *keys;
	void **values;
	struct BTreeNode **children;
	int isLeaf;
	struct BTreeNode *next;
	struct BTreeNode *parent;
} BTreeNode;

typedef struct BTree {
	BTreeNode *root;
} BTree;

BTreeNode *createBTreeNode(int isLeaf) {
	BTreeNode *node = (BTreeNode *)malloc(sizeof(BTreeNode));
	if(!node) {
		fprintf(stderr, "Malloc failed");
		exit(EXIT_FAILURE);
	};
	node->numKeys = 0;
	node->isLeaf = isLeaf;
	node->parent = NULL;
	node->keys = (int *)malloc(sizeof(int) * (2 * DEGREE - 1));
	if (isLeaf) {
		node->values = (void **)malloc(sizeof(void *) * (2 * DEGREE - 1));
		node->children = NULL;
		node->next = NULL;

	} else {
		node->values = NULL;
		node->children = (BTreeNode **)malloc(sizeof(BTreeNode *) * (2 * DEGREE));
		node->next = NULL;

		for (int i = 0; i < 2 * DEGREE; i++) {
		    node->children[i] = NULL;
		}
	}


	return node;
}


/*void splitChild(BTreeNode *parent, int idx, BTreeNode *child) {*/
/*		int mid = DEGREE - 1;*/
/*		BTreeNode *newNode = createbTreeNode(child-> isLeaf);*/
/*		newNode->numKeys = DEGREE - 1;*/
/**/
/*		for(int i = 0; i < DEGREE - 1; i++) {*/
/*			newNode->keys[i] = child->keys[i+ DEGREE];*/
/*			newNode->values[i] = child->values[i+DEGREE];*/
/*		}*/
/**/
/**/
/**/
/*}*/

int findChildIndex(BTreeNode *node, int key) {
	int left = 0;

	if (key < node->keys[left]) {
		return left;
	}

	int right = node->numKeys-1, mid;

	if (key > node->keys[right]) {
		return right + 1;
	}


	while(left <= right) {
		mid = (left + (right - left) ) / 2;

		if (node->keys[mid] == key) {
			return mid + 1;
		} else if (node->keys[mid] < key) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}
	return left;
}

BTreeNode *splitLeaf(BTreeNode *node, int key, void *value) {
	int i = node->numKeys - 1;
	while(node->keys[i] > key) {
		node->keys[i+1] = node->keys[i];
		node->values[i+1] = node->values[i];
		i--;

	}
	node->keys[i] = key;
	node->values[i] = value;
	node->numKeys++;
	int mid = DEGREE - 1;

	BTreeNode *second_half = createBTreeNode(1);
	for(int j = 0; j+mid+1 < node->numKeys; j++) {
		second_half->keys[j]  = node->keys[j + mid  + 1];
		second_half->values[j]  = node->values[j + mid  + 1];
	}
	node->numKeys = mid;
	second_half->numKeys = DEGREE;

	// Maintain leaf link pointers
	second_half->next = node->next;
	node->next = second_half;

	// Assign parent reference
	second_half->parent = node->parent;
	
	return second_half;
}

void insertIntoLeaf(BTreeNode *node, int key, void *value){
	int i = node->numKeys - 1;
	while(i >= 0 && key < node->keys[i]) {
		node->keys[i+1] = node->keys[i];
		node->values[i+1] = node->values[i];
		i--;
	}

	node->keys[i+1] = key;
	node->values[i+1] = value;
	node->numKeys++;
}



BTreeNode *insert(BTreeNode **root, BTreeNode*node, int key, void *value) {

	int childIndex = findChildIndex(node, key);

	if (!node->isLeaf) {
		BTreeNode *promoted = insert(root, node->children[childIndex], key, value);
		if (promoted) {
			int i;
				// add promoted to children
			i = node->numKeys-1;
			int new_key = promoted->keys[0];
			while(i >= 0 && new_key < node->keys[i]) {
				node->keys[i+1] = node->keys[i];
				node->children[i+2] = node->children[i+1];
				i--;
			}

			node->keys[i+1] = new_key;
			node->children[i+2] = promoted;
			node->numKeys++;

			if (node->numKeys < (2 * DEGREE - 1) ) return NULL; // normal insert
			// split node into half

			i = 1;
			int mid = DEGREE - 1;

			node->numKeys = mid;

			BTreeNode *second_half = createBTreeNode(0);
			second_half->isLeaf = node->isLeaf;
			second_half->numKeys = (2 * DEGREE - 1) - mid - 1;

			while (i < 2 * DEGREE - 1) {
				second_half->keys[i-mid-1] = node->keys[i];
				second_half->children[i-mid] = node->children[i];
				i++;
			}

			if (node->parent) {
				// later
				second_half->parent = node->parent;
				return second_half;

			} else {
				// create a new root
				BTreeNode *new_root = createBTreeNode(0);
				new_root->numKeys = 1;
				new_root->keys[0] = node->keys[mid];
				new_root->children[0] = node;
				new_root->children[1] = second_half;
				// fix parents
				node->parent = new_root;
				second_half->parent = new_root;
				// update global root pointer
				*root = new_root;
			}
		}
		return NULL;
	} else {
		if (node->numKeys < (2 * DEGREE - 1) ) { // normal insert
			insertIntoLeaf(node, key, value);
			return NULL;
		} else {
			return splitLeaf(node, key, value);
		}
	}
}

int main () {
	BTreeNode *root = createBTreeNode(1);
	int *value = malloc(sizeof(int));
	*value = 99;
	insert(&root, root, 1, value);
	for(int i = 0; i < root->numKeys; i++) {
		printf("key: %i | value: %i \n", root->keys[i], * (int *) root->values[i]);
	}

}


/*void insert(BTreeNode *node, int key, void *value) {*/
/*    if (node->isLeaf) {*/
/*        // Insert key in sorted order*/
/*        // If overflow, split and return promoted key*/
/*    } else {*/
/*        // Find the correct child*/
/*        // Recursively insert key into child*/
/*        // Handle promoted value if the child splits*/
/*        // If this node overflows, split and promote again*/
/*    }*/
/*}*/
