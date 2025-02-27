#include <stdio.h>
#include <stdlib.h>

#define DEGREE 3
#define MAXIMUM_KEYS ((DEGREE * 2) - 1)
#define MAXIMUM_CHILDREN ((DEGREE * 2))
#define MID ((MAXIMUM_KEYS) / 2)


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

void traverseBTree(BTreeNode *node, int depth) ;
BTreeNode *createBTreeNode(int isLeaf) {
	BTreeNode *node = (BTreeNode *)malloc(sizeof(BTreeNode));
	if(!node) {
		fprintf(stderr, "Malloc failed");
		exit(EXIT_FAILURE);
	};
	node->numKeys = 0;
	node->isLeaf = isLeaf;
	node->parent = NULL;
	node->keys = (int *)malloc(sizeof(int) * (MAXIMUM_KEYS));
	if (isLeaf) {
		node->values = (void **)malloc(sizeof(void *) * (MAXIMUM_KEYS));
		node->children = NULL;
		node->next = NULL;

	} else {
		node->values = NULL;
		node->children = (BTreeNode **)malloc(sizeof(BTreeNode *) * ( MAXIMUM_CHILDREN ));
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

void promote_to_parent(BTreeNode** root, BTreeNode* node, int promotedKey, BTreeNode *promoted) {
	if(!node->parent) {
		*root = createBTreeNode(0); // init new parent
		printf("promoted_k %i\n", promotedKey);
		(*root)->keys[0] = promotedKey;
		(*root)->children[0] = node;
		(*root)->children[1] = promoted;
		(*root)->numKeys = 1;
		node->parent = *root;
		promoted->parent = *root;

	} else {
		printf("masuk\n");
		BTreeNode *parent = node->parent;
		/*printf("before shifting~~~~~~~~~~~~~~~~~~~~\n");*/
		/*	traverseBTree(*root, 0);*/
		/*printf("~~~~~~~~~~~~~~~~~~~~~\n");*/
		/*printf("before shifting promoted~~~~~~~~~~~~~~~~~~~~\n");*/
		/*	traverseBTree(promoted, 0);*/
		/*printf("~~~~~~~~~~~~~~~~~~~~~\n");*/
		/*printf("parent of promoted~~~~~~~~~~~~~~~~~~~~\n");*/
		/*	traverseBTree(promoted->parent, 0);*/
		/*printf("~~~~~~~~~~~~~~~~~~~~~\n");*/
		/**/
		/*printf("parent of node~~~~~~~~~~~~~~~~~~~~\n");*/
		/*	traverseBTree(node->parent, 0);*/
		/*printf("~~~~~~~~~~~~~~~~~~~~~\n");*/
		/*printf("Before shifting parent keys: %i \n", parent->numKeys);*/
		/*for (int k = 0; k < parent->numKeys; k++) {*/
		/*    printf("%d ", parent->keys[k]);*/
		/*}*/
		/*printf("Parent children before shifting: ");*/
		/*for (int k = 0; k < parent->numKeys+1; k++) {*/
		/*    printf("%p ", (void*)parent->children[k]);*/
		/*}*/
		int i = parent->numKeys;
		while (i > 0 && parent->keys[i - 1] > promotedKey) {
		    parent->keys[i] = parent->keys[i - 1];
		    parent->children[i + 1] = parent->children[i];
		    i--;
		}

		parent->keys[i] = promotedKey;
		parent->children[i + 1] = promoted;
		parent->numKeys++;


		if(parent->numKeys >= MAXIMUM_KEYS) {    
		    BTreeNode *second_half = createBTreeNode(0);
		    int midIndex = parent->numKeys / 2;
		    
		    // Save the middle key that will be promoted
		    int new_promoted = parent->keys[midIndex];
		    
		    // Set up second_half children and keys
		    second_half->children[0] = parent->children[midIndex + 1];
		    second_half->children[0]->parent = second_half;
		    
		    for (int j = midIndex + 1; j < parent->numKeys; j++) {
			second_half->keys[j - (midIndex + 1)] = parent->keys[j];
			second_half->children[j - midIndex] = parent->children[j + 1];
		        second_half->children[j - midIndex]->parent = second_half;
		    }
		    
		    // Set the numKeys correctly for both halves
		    second_half->numKeys = parent->numKeys - (midIndex + 1);
		    parent->numKeys = midIndex;
		    
		    // Set up the promoted node with the middle key
		    second_half->parent = parent->parent;
			
		    
		    // Promote to parent
		    promote_to_parent(root, parent, new_promoted, second_half);
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
			promoted_pointer = createBTreeNode(1); // Create new leaf node

			int midIndex = node->numKeys / 2;
			int promoted_val = node->keys[midIndex]; // Get the middle key (stays in leaf)

			// Move right half to new leaf node
			for (int k = midIndex; k < node->numKeys; k++) { // Start from midIndex (not +1!)
			    promoted_pointer->keys[k - midIndex] = node->keys[k];
			    promoted_pointer->values[k - midIndex] = node->values[k];
			}

			promoted_pointer->numKeys = node->numKeys - midIndex; // Right half keys count
			node->numKeys = midIndex; // Left half keys count

			// 🌟 Update leaf links for B+Tree

			// Set parent
			promoted_pointer->parent = node->parent;


			promote_to_parent(root, node, promoted_val, promoted_pointer);
			return;

			// promote to parent -> parent empty, parent, full

		}

	}


}

int expected = 0;
void traverseBTree(BTreeNode *node, int depth) {
	if (node == NULL) return;

	// Print left children
	if(node->isLeaf) {
		for (int i = 0; i < node->numKeys; i++) {
			printf("Depth %d: index = %d | key %i | Value: %i\n", depth, i, node->keys[i], * (int *) node->values[i]);
			if(* (int *) node->values[i] != expected) exit(0);
			if(node->keys[i] != expected) exit(0);
			if(expected == 2499) {
				expected += 2; 
			} else expected++;

		}

	} else {
		traverseBTree(node->children[0], depth + 1);
		printf("Depth %d: index = %d | key %i \n", depth, 0, node->keys[0]);
		for (int i = 1; i <= node->numKeys; i++) {
			traverseBTree(node->children[i], depth + 1);
			if (i < node->numKeys) printf("Depth %d: index = %d | key %i \n", depth, i, node->keys[i]);
		}
	}

}

void get(BTreeNode *node, int key) {
	if (!node) return;

	BTreeNode *current_node = node;
	int idx;

	// Traverse down to the leaf node
	while (!current_node->isLeaf) {
		idx = findChildIndex(current_node, key);
		current_node = current_node->children[idx];
	}

	int left = 0;
	int right = current_node->numKeys - 1;

	if (key < current_node->keys[left]) {
	    printf("Key %d not found\n", key);
	    return;
	}

	int mid;

	if (key > current_node->keys[right]) {
	    printf("Key %d not found\n", key);
	    return;
	}


	while(left <= right) {
		mid = left + ((right - left)  / 2 );

		if (current_node->keys[mid] == key) {
			break;
		} else if (current_node->keys[mid] < key) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}
	if (current_node->keys[mid] == key) {
		printf("Found key: %i, value: %i\n", key, * (int *) current_node->values[mid]);
		return;
	}


	printf("Key %d not found, mid: %i\n", key, mid);

}

void printBTree(BTreeNode *node, int depth) {
    if (!node) return;

    // Print current node keys
    printf("Depth %d: ", depth);
    for (int i = 0; i < node->numKeys; i++) {
	if(!node->isLeaf)
		printf("%d ", node->keys[i]);
	else
        printf("( %d %i ) [addr: %p] ", node->keys[i], * (int *) node->values[i] , node->values[i]);
    }
    printf("\n");

    // Recursively print child nodes
    if (!node->isLeaf) {
        for (int i = 0; i <= node->numKeys; i++) {
            printBTree(node->children[i], depth + 1);
        }
    }
}

int main () {
	BTreeNode *root2 = createBTreeNode(1);

	int l = 0;
	int r = 5000;
	while (l < r) {
		int *val = malloc(sizeof(int));
		int *vval = malloc(sizeof(int));
		if(!val) printf("malloc failed");
		if(!vval) printf("malloc failed");
;
		*val = l;
		*vval = r;
		/*printf("val: %i addr %p \n", *val, val);*/

		insert(&root2, root2, (*vval), vval);
		insert(&root2, root2, (*val), val);
		l++;
		r--;
	}
	/*printBTree(root2, 0);*/
	traverseBTree(root2, 0);
	printf("\nexpected leggo\n");
	for(int i = -2 ; i <= 5002; i++) {
		get(root2, i);
	}
	/*get(root2, 2500);*/
	/*get(root2, 0);*/
	/*get(root2, 2499);*/
	/*get(root2, 5001);*/
	/*get(root2, 5000);*/
	/*get(root2, 2501);*/


	/*printBTree(root2, 0);*/



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
