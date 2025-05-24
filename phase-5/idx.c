#include "idx.h"
#include <stdio.h>
#include <stdlib.h>

#define DEGREE 3
#define MAXIMUM_KEYS ((DEGREE * 2) - 1)
#define MAXIMUM_CHILDREN ((DEGREE * 2))
#define MID ((MAXIMUM_KEYS) / 2)
#define MINIMUM_KEYS (DEGREE - 1)


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
		node->prev = NULL;

	} else {
		node->values = NULL;
		node->children = (BTreeNode **)malloc(sizeof(BTreeNode *) * ( MAXIMUM_CHILDREN ));
		node->next = NULL;
		node->prev = NULL;

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
		(*root)->keys[0] = promotedKey;
		(*root)->children[0] = node;
		(*root)->children[1] = promoted;
		(*root)->numKeys = 1;
		node->parent = *root;
		promoted->parent = *root;

	} else {
		BTreeNode *parent = node->parent;

		int i = parent->numKeys;
		while (i > 0 && parent->keys[i - 1] > promotedKey) {
		    parent->keys[i] = parent->keys[i - 1];
		    parent->children[i + 1] = parent->children[i];
		    i--;
		}

		parent->keys[i] = promotedKey;
		parent->children[i + 1] = promoted;
		promoted->parent = parent;
		parent->numKeys++;


		if(parent->numKeys >= MAXIMUM_KEYS) {    
		    BTreeNode *second_half = createBTreeNode(0);
		    
		    int new_promoted = parent->keys[MID];
		    
		    second_half->children[0] = parent->children[MID + 1];
		    second_half->children[0]->parent = second_half;
		    
		    for (int j = MID + 1; j < parent->numKeys; j++) {
			second_half->keys[j - (MID + 1)] = parent->keys[j];
			second_half->children[j - MID] = parent->children[j + 1];
		        second_half->children[j - MID]->parent = second_half;
		    }
		    
		    second_half->numKeys = parent->numKeys - (MID + 1);
		    parent->numKeys = MID;
		    
		    second_half->parent = parent->parent;
			
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
			promoted_pointer = createBTreeNode(1); 

			int promoted_val = node->keys[MID]; 

			for (int k = MID; k < node->numKeys; k++) { 
			    promoted_pointer->keys[k - MID] = node->keys[k];
			    promoted_pointer->values[k - MID] = node->values[k];
			}

			promoted_pointer->numKeys = node->numKeys - MID;
			node->numKeys = MID; 

			promoted_pointer->parent = node->parent;
			promoted_pointer->next = node->next;
			promoted_pointer->prev = node;

			if (node->next != NULL) {
			    node->next->prev = promoted_pointer;  // fix the old next leaf's prev pointer
			}

			node->next = promoted_pointer;


			promote_to_parent(root, node, promoted_val, promoted_pointer);
			return;
		}
	}
}

void traverseBTree(BTreeNode *node, int depth) {
	if (node == NULL) return;

	// Print left children
	if(node->isLeaf) {
		for (int i = 0; i < node->numKeys; i++) {
			printf("Depth %d: index = %d | key %i | Value: %i\n", depth, i, node->keys[i], * (int *) node->values[i]);

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

void debugParentPointers(BTreeNode *node, int depth); 
void destroy(BTreeNode **root, BTreeNode *node, int key) {
	// internal node
	if(!node->isLeaf) {
		int childIndex = findChildIndex(node, key);
		destroy(root, node->children[childIndex], key);

		BTreeNode *destroyed_child = node->children[childIndex];

		if (destroyed_child ->numKeys < MINIMUM_KEYS) {
			// we try to borrow from right and then from left
			// if right and left has minimum keys too we try to merge from right to left
			int right = childIndex + 1;
			// if child index is not foremost right
			if(childIndex < node->numKeys &&  node->children[right]->numKeys > MINIMUM_KEYS ) {
				BTreeNode *right_child = node->children[right];

				if(!right_child->isLeaf) { // if children is not leaf, we shift the grandchildren of children 
					//
					// claim right children to borrower
					destroyed_child->children[destroyed_child->numKeys+1] = right_child->children[0];
					destroyed_child->children[destroyed_child->numKeys+1]->parent = destroyed_child;
					// claim parent key to borrower
					destroyed_child->keys[destroyed_child->numKeys] = node->keys[childIndex];
					// claim first key of right as the new separator
					node->keys[childIndex] = right_child->keys[0];

					// shift right children
					int i = 1;

					while (i < right_child->numKeys) {
						right_child->keys[i - 1] = right_child->keys[i];
						right_child->children[i-1] = right_child->children[i];
						i++;
					}
					right_child->children[i - 1] = right_child->children[i];
				} else {
					// claim right values to borrower
					destroyed_child->values[destroyed_child->numKeys] = right_child->values[0];
					// in this case right_child->keys[0] == node->keys[childIndex]
					destroyed_child->keys[destroyed_child->numKeys] = right_child->keys[0];
					// claim first key of right as the new separator
					node->keys[childIndex] = right_child->keys[1];

					// shift right values
					int i = 1;

					while (i < right_child->numKeys) {
						right_child->keys[i - 1] = right_child->keys[i];
						right_child->values[i-1] = right_child->values[i];
						i++;
					}
				}
				right_child->numKeys--;
				destroyed_child->numKeys++;

			}else if( childIndex > 0 &&  node->children[childIndex - 1]->numKeys > MINIMUM_KEYS ) {
				BTreeNode *left_child = node->children[childIndex - 1];
				int i = destroyed_child->numKeys;
				if(!left_child->isLeaf) { // if children is not leaf, we shift the grandchildren of children 
					//
					// shift destroyed to right to make space
					// shift keys and children to right to make space
					destroyed_child->children[i+1] = destroyed_child->children[i];
					while (i > 0) {
						destroyed_child->keys[i] = destroyed_child->keys[i-1];
						destroyed_child->children[i] = destroyed_child->children[i-1];
						i--;
					}
					// claim left
					destroyed_child->keys[0] = node->keys[childIndex-1];
					destroyed_child->children[0] = left_child->children[left_child->numKeys];
					destroyed_child->children[0]->parent = destroyed_child;
					//claim foremost right children of left
					node->keys[childIndex-1] = left_child->keys[left_child->numKeys -1];

				} else {
					while (i > 0) {
						destroyed_child->keys[i] = destroyed_child->keys[i-1];
						destroyed_child->values[i] = destroyed_child->values[i-1];
						i--;
					}
					destroyed_child->keys[0] = left_child->keys[left_child->numKeys-1];
					destroyed_child->values[0] = left_child->values[left_child->numKeys-1];
					node->keys[childIndex-1] = destroyed_child->keys[0];
				}
				left_child->numKeys--;
				destroyed_child->numKeys++;
			// merge with right if right is minimum
			} else if (childIndex < node->numKeys ) {
				BTreeNode *right_child = node->children[right];

				if(!right_child->isLeaf) {
					destroyed_child->keys[destroyed_child->numKeys] = node->keys[childIndex];
					for(int i = 0; i < right_child->numKeys; i++) {
						destroyed_child->keys[destroyed_child->numKeys + 1 + i] = right_child->keys[i];
						destroyed_child->children[destroyed_child->numKeys + 1 + i] = right_child->children[i]; // this stops at destroyed_child->num
						right_child->children[i]->parent = destroyed_child; //update parent on merge
					}
					destroyed_child->children[destroyed_child->numKeys + right_child->numKeys + 1] = right_child->children[right_child->numKeys];
					right_child->children[right_child->numKeys]->parent = destroyed_child; //update parent on merge
					destroyed_child->numKeys += right_child->numKeys + 1;

				} else {
					for(int i = 0; i < right_child->numKeys; i++) {
						destroyed_child->keys[destroyed_child->numKeys + i] = right_child->keys[i];
						destroyed_child->values[destroyed_child->numKeys + i] = right_child->values[i];
					}
					destroyed_child->numKeys += right_child->numKeys;

				}

				for(int i = childIndex; i < node->numKeys - 1; i++) {
					node->keys[i] = node->keys[i+1];
					node->children[i+1] = node->children[i+2];
				}
				node->numKeys--;

				// shift keys and children after destroyed
				// add free malloc in right child later


			} else if (childIndex > 0 ){
				BTreeNode *left_child = node->children[childIndex - 1];

				if(!left_child->isLeaf) {
					left_child->keys[left_child->numKeys] = node->keys[childIndex-1];
					for(int i = 0; i < destroyed_child->numKeys; i++) {
						left_child->keys[left_child->numKeys + 1 + i] = destroyed_child->keys[i];
						left_child->children[left_child->numKeys + 1 + i] = destroyed_child->children[i]; // this stops at destroyed_child->num
						destroyed_child->children[i]->parent = left_child; //update parent on merge
					}
					left_child->children[destroyed_child->numKeys + left_child->numKeys + 1] = destroyed_child->children[destroyed_child->numKeys];
					destroyed_child->children[destroyed_child->numKeys]->parent = left_child; //update parent on merge
					left_child->numKeys += destroyed_child->numKeys + 1;
				} else {
					for(int i = 0; i < destroyed_child->numKeys; i++) {
						left_child->keys[left_child->numKeys + i] = destroyed_child->keys[i];
						left_child->values[left_child->numKeys + i] = destroyed_child->values[i];
					}
					left_child->numKeys += destroyed_child->numKeys ;
				}

				for(int i = childIndex; i < node->numKeys; i++) {
					node->keys[i-1] = node->keys[i];
					node->children[i] = node->children[i+1];
				}
				node->numKeys--;
			} 

			if(!node->parent && node->numKeys == 0) {
				node->children[0]->parent = NULL;
				*root = node->children[0];
			}
		}
	} else {
		int left = 0;
		int right = node->numKeys - 1;

		if (key < node->keys[left]) {
		    printf("Key %d not found\n", key);
		    return;
		}

		int mid;

		if (key > node->keys[right]) {
		    printf("Key %d not found\n", key);
		    return;
		}


		while(left <= right) {
			mid = left + ((right - left)  / 2 );

			if (node->keys[mid] == key) {
				break;
			} else if (node->keys[mid] < key) {
				left = mid + 1;
			} else {
				right = mid - 1;
			}
		}
		if (node->keys[mid] == key) {
			int i = mid;
			while(i < node->numKeys-1) {
				node->keys[i] = node->keys[i+1];
				node->values[i] = node->values[i+1];
				i++;

			}
			node->numKeys--;
			if(mid == 0) {
				BTreeNode *parent = node->parent;
				while (parent) {
					int childIdx = findChildIndex(parent, node->keys[mid]);
					if(childIdx > 0) {
						parent->keys[childIdx-1] = node->keys[0];
						break;
					}
					parent = parent->parent;
				}
			}
			return;
		}


		printf("Key %d not found, mid: %i\n", key, mid);

	}
}
void debugParentPointers(BTreeNode *node, int depth) {
    if (!node) return;

    printf("Depth %d: Node at %p, Parent at %p, children num: %i\n", depth, (void *)node, (void *)node->parent, node->numKeys + 1);
    
if (!node->isLeaf) {  
        for (int i = 0; i <= node->numKeys; i++) {
            if (node->children[i] != NULL) {
                debugParentPointers(node->children[i], depth + 1);
            }
        }
    }
}
/*int main () {*/
/*	// this code still has not handle duplicates yet*/
/*	BTreeNode *root2 = createBTreeNode(1);*/
/**/
/*/*	int l = 0;*/
/*/*	int r = 50;*/
/*	while (l < r) {*/
/*		int *val = malloc(sizeof(int));*/
/*		int *vval = malloc(sizeof(int));*/
/*		if(!val) printf("malloc failed");*/
/*		if(!vval) printf("malloc failed");*/
/*;*/
/*		*val = l;*/
/*		*vval = r;*/
/*		/*printf("val: %i addr %p \n", *val, val);*/
/**/
/*		insert(&root2, root2, (*vval), vval);*/
/*		insert(&root2, root2, (*val), val);*/
/*		l++;*/
/*		r--;*/
/*	}*/
/*	int values[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 25, 28, 0, 42, 69, 71, 2, 5, 12, 31, 24};*/
/*    for (int i = 0; i < 25; i++) {*/
/*        int *vval = malloc(sizeof(int));*/
/*        *vval = values[i];*/
/*        insert(&root2, root2, *vval, vval);*/
/*	    printf("\nTree after inserting %i:\n", values[i]);*/
/*	    traverseBTree(root2, 0);*/
/*    }*/
/**/
/**/
/*    int toDelete[] = {30, 40, 50, 10, 20, 2, 120};*/
/*    for (int i = 0; i < 7; i++) {*/
/*        printf("\nDeleting %d...\n", toDelete[i]);*/
/*        destroy(&root2, root2, toDelete[i]);*/
/*	debugParentPointers(root2, 0);*/
/**/
/*        traverseBTree(root2, 0);*/
/*    }*/
/**/
/*    printf("\nFinal tree structure:\n");*/
/*    traverseBTree(root2, 0);*/
	/*debugParentPointers(root2, 0);*/
	/*int *val = malloc(sizeof(int));*/
	/**val = 25;*/
	/*insert(&root2, root2, (*val), val);*/
	/*printf("=====after last insertion====");*/
	/*debugParentPointers(root2, 0);*/
	/*destroy(&root2, root2, 3);*/
	/*debugParentPointers(root2, 0);*/
	/*destroy(&root2, root2, 48);*/
	/*destroy(&root2, root2, 49);*/
	/*debugParentPointers(root2, 0);*/
	/*destroy(&root2, root2, 12);*/
	/*debugParentPointers(root2, 0);*/
	/*destroy(&root2, root2, 13);*/
	/*debugParentPointers(root2, 0);*/
	/*destroy(&root2, root2, 14);*/
	/*debugParentPointers(root2, 0);*/
	/*destroy(&root2, root2, 3);*/
	/*destroy(&root2, root2, 18);*/
	/*debugParentPointers(root2, 0);*/
	/*destroy(&root2, root2, 3);*/
	/*printf("\nexpected leggo?\n");*/
	/*traverseBTree(root2, 0);*/
	/*printf("\nexpected leggo\n");*/
	/*get(root2, 2500);*/
	/*get(root2, 0);*/
	/*get(root2, 2499);*/
	/*get(root2, 5001);*/
	/*get(root2, 5000);*/
	/*get(root2, 2501);*/


	/*printBTree(root2, 0);*/



/*}*/


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
