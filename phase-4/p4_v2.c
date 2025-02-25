#include <stdio.h>
#include <stdlib.h>

#define DEGREE 10
#define MAXIMUM_KEYS ((DEGREE * 2) - 1)
#define MAXIMUM_CHILDREN ((DEGREE * 2))
#define MID ((MAXIMUM_KEYS + 1) / 2)


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


int findChildIndex(BTreeNode *node, int key) {
	int left = 0;

	if (key < node->keys[left]) {
		return left;
	}

	int right = node->numKeys - 1;
	int mid;

	if (key > node->keys[right]) {
		return right + 1;
	}


	while(left <= right) {
		mid = left + ((right - left)  / 2 );

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

void promote_to_parent(BTreeNode** root, BTreeNode* node, BTreeNode *promoted) {
	if(!node->parent) {
		*root = createBTreeNode(0); // init new parent
		(*root)->keys[0] = promoted->keys[0];
		(*root)->children[0] = node;
		(*root)->children[1] = promoted;
		(*root)->numKeys = 1;
		node->parent = *root;
		promoted->parent = *root;

	} else {
		BTreeNode *parent = node->parent;
		int i = parent->numKeys;

		while(i > 0 && parent->keys[i-1] > promoted->keys[0]) {
		    parent->keys[i] = parent->keys[i-1];
		    parent->children[i+1] = parent->children[i];
		    i--;
		}
		i++;

		parent->keys[i-1] = promoted->keys[0];
		parent->children[i] = promoted;
		parent->numKeys++;


		if(parent->numKeys >= MAXIMUM_KEYS) {	


			BTreeNode *second_half = createBTreeNode(0);
			second_half->children[0] = parent->children[MID];
			second_half->children[0]->parent = second_half;

			for (int j = MID; j < parent->numKeys; j++) {
				second_half->keys[j - MID] = parent->keys[j];
				second_half->children[j - MID + 1] = parent->children[j + 1];
				second_half->children[j - MID + 1]->parent = second_half;

			}
			second_half->parent = parent->parent;
			second_half->numKeys = parent->numKeys - MID;
			parent->numKeys = MID;

			promote_to_parent(root, parent, second_half);





			// split in half and promote_to parent
		}
	}
}

void insert(BTreeNode **root, BTreeNode*node, int key, void *value) {
	BTreeNode *promoted_pointer = NULL;

	if(!node->isLeaf) {
		int childIndex = findChildIndex(node, key);
		insert(root, node->children[childIndex], key, value);

	} else {
		int i = node->numKeys - 1;
		while(i >= 0 && key < node->keys[i]) {
			node->keys[i+1] = node->keys[i];
			node->values[i+1] = node->values[i];
			i--;
		}
		node->keys[i+1] = key;
		node->values[i+1] = value;
		node->numKeys++;

		if(node->numKeys >= MAXIMUM_KEYS) {
			promoted_pointer = createBTreeNode(1);
			promoted_pointer->parent = node->parent;

			int k = node->numKeys - 1;

			for(int k = node->numKeys - 1; k >= MID; k--) {
				promoted_pointer->keys[k-MID] = node->keys[k];
				promoted_pointer->values[k-MID] = node->values[k];
			}

			promoted_pointer->numKeys = node->numKeys - MID;
			node->numKeys = MID;

			promote_to_parent(root, node, promoted_pointer);

			// promote to parent -> parent empty, parent, full

		}

	}


}


void traverseBTree(BTreeNode *node, int depth) {
	printf("<<<\n");
	if (node == NULL) return;

	// Print left children
	if(node->isLeaf) {
		for (int i = 0; i < node->numKeys; i++) {
			printf("Depth %d: Key = %d\n", depth, node->keys[i]);
		}

	} else {
		for (int i = 0; i < node->numKeys+1; i++) {
			traverseBTree(node->children[i], depth + 1);
		}
	}

}

void get(BTreeNode *node, int key) {
    if (!node) return;

    BTreeNode *current_node = node;
    int idx;

    // Traverse down to the leaf node
    while (!current_node->isLeaf) {
	printf("zz\n");
        idx = findChildIndex(current_node, key);
	printf("xx\n");
        current_node = current_node->children[idx];
    }

    // Find the key in the leaf node
    for (idx = 0; idx < current_node->numKeys; idx++) {
        if (current_node->keys[idx] == key) {
            printf("Found key: %d, value: %d\n", key, *(int *) current_node->values[idx]);
            return;
        }
    }

    printf("Key %d not found\n", key);

}

int main () {
	BTreeNode *root = createBTreeNode(1);
	BTreeNode *root2 = createBTreeNode(1);
/*	for(int i = 500; i >= 0; i--) {*/
/**/
/*		int *val = malloc(sizeof(int));*/
/*;*/
/*		*val = i;*/
/*		insert(&root2, root2, *val, val);*/
/*	}*/
	for(int i = 1; i <= 5000; i*=3) {

		int *val = malloc(sizeof(int));
;
		*val = i;
		insert(&root2, root2, (*val)/3 , val);
	}

	/*traverseBTree(root2, 0);*/
	get(root2, 9);



	/*for(int i = 0; i <= (root)->numKeys; i++) {*/
	/*	BTreeNode *child = (root)->children[i];*/
	/**/
	/**/
	/*		for(int j = 0; j < child->numKeys; j++) {*/
	/*		}*/
	/*}*/

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
