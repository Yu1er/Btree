#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <stdio.h>
#include <time.h>
#define M 128
#define T 64

typedef struct btreenode {
    int keys[M - 1];
    void* values[M - 1];
    struct btreenode* children[M];
    int nOKeys;
    int nOChildren;
} btnode;
typedef struct tree {
    int m;
    int t;
    btnode* root;
} btree;

// create functions
void initTree(btree* tree); 
btnode* btree_createNode(btree* tree);
btnode* btree_createNodeWithVal(btree* tree, int key, void* value);

// main operations
void btree_insert(btree* tree, int key, void* value);
btnode* btree_insertIntoNode(btree* tree, btnode* node, int key, void* value);
int btree_search(btnode* root, int key);

// support functions
int binarySearchPos(btnode* node, int key);
void btree_splitNode(btree* tree, btnode* node, btnode* newRoot, int pos);
void printBtree(btnode* root);
void moveKeyAndVal(btnode* from, int fromIdx, btnode* to, int toIdx);

// free tree 
void freeTree(btnode* root);

// 验证 B 树结构的函数
void verifyBTree(btnode* node, int t, int m, bool is_root) {
    if (node == NULL) return;
    // 验证关键字数量
    if (is_root) {
        if (node->nOKeys < 1) {
            printf("Error: Root node has no keys.\n");
            exit(1);
        }
    }
    else{
        if (node->nOKeys < t - 1 && node != NULL) {
            printf("Error: Node has invalid number of keys.\n");
            exit(1);
        }
        if (node->nOKeys > m - 1) {
            printf("Error: Node has too many keys.\n");
            exit(1);
        }
    }
    // 验证子节点数量
    if (node->nOChildren > m) {
        printf("Error: Node has too many children.\n");
        exit(1);
    }
    // 验证关键字顺序
    for (int i = 0; i < node->nOKeys - 1; i++) {
        if (node->keys[i] >= node->keys[i + 1]) {
            printf("Error: Keys are not in ascending order.\n");
            exit(1);
        }
    }
    // 递归验证子节点
    for (int i = 0; i < node->nOChildren; i++) {
        verifyBTree(node->children[i], t, m, false);
    }
}

int main(void) {
    // 初始化 B 树
    btree tree;
    initTree(&tree);
    // 插入测试
    printf("Inserting elements...\n");
    int insertElements[] = {10, 88, 30, 40, 34, 373, 3, 80, 90, 81};
    int insertLen = sizeof(insertElements) / sizeof(insertElements[0]);
    for (int i = 0; i < insertLen; i++) {
        btree_insert(&tree, insertElements[i], NULL);
    }
    printf("Tree after insertions:\n");
    printBtree(tree.root);
    printf("\n");
    // 查找测试
    printf("Searching for elements...\n");
    int searchElements[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 150};
    int searchLen = sizeof(searchElements) / sizeof(searchElements[0]);
    for (int i = 0; i < searchLen; i++) {
        int result = btree_search(tree.root, searchElements[i]);
        if (result == 0) {
            printf("Key %d found.\n", searchElements[i]);
        } else {
            printf("Key %d not found.\n", searchElements[i]);
        }
    }
    // 随机插入测试
    printf("Random insertions...\n");
    srand(time(NULL));
    int randomOperations = 1000;
    for (int i = 0; i < randomOperations; i++) {
        int randomKey = rand() % 1000;
        btree_insert(&tree, randomKey, NULL);
    }
    printf("Tree after random insertions:\n");
    printBtree(tree.root);
    printf("\n");
    // 验证树的结构
    printf("Verifying tree structure...\n");
    verifyBTree(tree.root, T, M, true);
    printf("Tree structure is valid.\n");

    // 释放树的内存
    freeTree(tree.root);

    return 0;
}

void initTree(btree* tree) {
    tree->m = M;
    tree->t = T;
    tree->root = NULL;
}

btnode* btree_createNode(btree* tree) {
    btnode* node = (btnode*)malloc(sizeof(btnode));
    node->nOKeys = 0;
    node->nOChildren = 0;
    return node;
}

btnode* btree_createNodeWithVal(btree* tree, int key, void* value) {
    btnode *node = btree_createNode(tree);
    node->keys[0] = key;
    node->values[0] = value;
    node->nOKeys = 1;
    return node;
}

void btree_insert(btree* tree, int key, void* value) {
    if (!tree->root) {
        tree->root = btree_createNodeWithVal(tree, key, value);
    }
    else {
        btnode* ret = btree_insertIntoNode(tree, tree->root, key, value);
        if (ret) {
            tree->root = ret;
        }
    }
}

btnode* btree_insertIntoNode(btree* tree, btnode* node, int key, void* value) {
    int pos = binarySearchPos(node, key);
    // pos points to a new suggested position, current keys[pos] is bigger than key
    // points to a proper child
    if (node->keys[pos] == key) {
        // key already inserted
        return NULL;
    }
    btnode* ret = NULL;
    bool is_leaf = !node->nOChildren;
    if (is_leaf) {
        // is a leaf -> insert here
        if (node->nOKeys == tree -> m - 1) {
            // will overflow -> store the key to be inserted in ret
            // ret will contain the value that is popped up and 2 children (first  node, second new)
            ret = btree_createNodeWithVal(tree, key, value);
            btree_splitNode(tree, node, ret, pos);
        }
        else {
            // has space -> just insert
            // shift keys to the right of pos to the right
            for (int i = node->nOKeys; i > pos; i--) {
                moveKeyAndVal(node, i - 1, node, i);
            }
            // insert key
            node->keys[pos] = key;
            node->values[pos] = value;
            node->nOKeys++;
        }
    }
    else {
        // has children -> insert into proper child, then see if a value popped up in ret and correctly insert it into this parent node
        ret = btree_insertIntoNode(tree, node->children[pos], key, value);
        if (ret) {
            // the child has overflown, the new value and splitted children have to be inserted in this node
            if (node->nOKeys == tree->m - 1) {
                // parent is full -> first split the parent
                // ret contains value to be inserted
                // splitNode will take care of inserting the value and dividing all the children
                btree_splitNode(tree, node, ret, pos);
            }
            else {
                // insert the popped up key, insert the children
                // right shift keys
                for (int i = node->nOKeys; i > pos; i--) {
                    moveKeyAndVal(node, i - 1, node, i);
                }
                // insert key
                node->keys[pos] = ret->keys[0];
                node->values[pos] = ret->values[0];
                // right shift children
                // pos + 1 because the child at pos is already equal to the child that we split, we do not need to move it
                for (int i = node->nOChildren; i > pos + 1; i--) {
                    node->children[i] = node->children[i - 1];
                }
                node->children[pos] = ret->children[0]; // useless but better for visualization
                node->children[pos + 1] = ret->children[1]; // puts the second half of the child we split
                // update counts
                node->nOKeys++;
                node->nOChildren++;
                // we have dealt with ret, now it needs to be emptied so we do not assign root to it
                free(ret);
                ret = NULL;
            }

        }
    }
    
    return ret;
}

int binarySearchPos(btnode* node, int key) {
    int left = 0;
    int right = node->nOKeys - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (node->keys[mid] == key) {
            return mid;
        }
        else if (node->keys[mid] > key) {
            right = mid - 1;
        }
        else {
            left = mid + 1;
        }
    }
    return left;
}

void btree_splitNode(btree* tree, btnode* node, btnode* newRoot, int pos) {
    // newRoot contains value we want to insert and then split, pos already has the position
    btnode* tmp = btree_createNode(tree);
    // if newRoot containts children then the oveflow from a child caused an overflow at node, so we will need to properly divide the children
    tmp->children[0] = newRoot->children[0];
    tmp->children[1] = newRoot->children[1];
    bool hasChildren = node->nOChildren;
    // first take care of keys
    // t - 1 is index of middle if we overflow (it is basically ceil(m / 2) - 1)
    if (pos < tree->t - 1) {
        // our value will end up in the left child, the middle will be an element at t - 2 as it will get shifted forwards
        // remember the to be popped up element in tmp
        moveKeyAndVal(node, tree->t - 2, tmp, 0);
        // right shift elements to fill the gap (which is current t - 2)
        for (int i = tree->t - 2; i > pos; i--) {
            moveKeyAndVal(node, i - 1, node, i);
        }
        // insert key
        moveKeyAndVal(newRoot, 0, node, pos);
    }
    else if (pos > tree->t - 1) {
        // our value will end up in the right child
        // the new middle will be current t - 1, so it will be deleted from the array
        // so pos we do not need to move the key at pos to the right but insert to the left of it as there will be guaranteed space
        // remember the element to be popped up in tmp
        moveKeyAndVal(node, tree->t - 1, tmp, 0);
        // left shift elements to fill in the gap at t - 1
        // pos - 1 as at pos there is an element that is more than key, so it has to stay to the right of it and not get shifted
        for (int i = tree->t - 1; i < pos - 1; i++) {
            node->keys[i] = node->keys[i + 1];
            moveKeyAndVal(node, i + 1, node, i);
        }
        // insert our key
        moveKeyAndVal(newRoot, 0, node, pos - 1);
    }
    else {
        // our key will be popped up
        tmp->keys[0] = newRoot->keys[0]; // useless but good for visualization
        moveKeyAndVal(newRoot, 0, tmp, 0);
    }
    // put the element that will be upshifted in the new root
    moveKeyAndVal(tmp, 0, newRoot, 0);
    newRoot->children[0] = node;
    newRoot->children[1] = btree_createNode(tree);
    // divide the keys between the children
    for (int i = tree->t - 1; i < tree->m - 1; i++) {
        moveKeyAndVal(newRoot->children[0], i, newRoot->children[1], i - tree->t + 1);
        newRoot->children[0]->keys[i] = INT_MAX;
    }
    if (hasChildren) {
        if (pos < tree->t - 1) {
            // copy children to the right side, child at t - 1 goes to the right, because the popped up value was at t - 2 -> child at t - 2 is the biggest one on the left
            for (int i = tree->t - 1; i < tree->m; i++) {
                newRoot->children[1]->children[i - tree->t + 1] = newRoot->children[0]->children[i];
            }
            // at left child shift children to the right to make space for the divided children of newRoot
            for (int i = tree->t - 1; i > pos + 1; i--) {
                newRoot->children[0]->children[i] = newRoot->children[0]->children[i - 1];
            }
            // insert the children from newRoot that we saved in tmp
            newRoot->children[0]->children[pos] = tmp->children[0]; // useless but anyway
            newRoot->children[0]->children[pos + 1] = tmp->children[1];
        }
        else {
            // we have to do almost the same thing with > t - 1 as with = t - 1
            // copy children to the right side, child at t - 1 stays on the left, as it after key t - 2, which stays on the left
            for (int i = tree->t; i < tree->m; i++) {
                newRoot->children[1]->children[i - tree->t] = newRoot->children[0]->children[i];
            }
            // at right child shift children to the right to make space for the divided children of newRoot, also remember real position is pos - 1
            // position of key in the right child is calculated as pos - 1 - (t - 1) = pos - t
            for (int i = tree->t; i > pos - tree->t + 1; i--) {
                newRoot->children[1]->children[i] = newRoot->children[1]->children[i - 1];
            }
            // insert the childrent from newRoot
            // if our key popped up into newRoot, it is useless but better for visualization to show that left child contains the 0 child of newRoot at index t - 1
            newRoot->children[1]->children[pos - tree->t + 1] = tmp->children[1];
            if (pos == tree->t) {
                newRoot->children[0]->children[tree->t - 1] = tmp->children[0]; // useless
            }
            else {
                newRoot->children[1]->children[pos - tree->t] = tmp->children[0]; // again, useless
            }
        }
        // update counts of children
        newRoot->children[0]->nOChildren = tree->t;
        newRoot->children[1]->nOChildren = tree->m - tree->t + 1;
    }
    // update counts of keys
    newRoot->children[0]->nOKeys = tree->t - 1;
    newRoot->children[1]->nOKeys = tree->m - tree->t;
    newRoot->nOChildren = 2;
    newRoot->nOKeys = 1;
    free(tmp);
}

void printBtree(btnode *root)
{
    if (root){
        printf("( ");
        for (int i = 0; i < root->nOKeys; i++){
            if (root->nOChildren){
                printBtree(root->children[i]);
            }
            printf(" %d ", root->keys[i]);
        }
        if (root->nOChildren){
            printBtree(root->children[root->nOKeys]);
        }

        printf(" )");
    }
    else{
        printf("<>");
    }
    
}

void freeTree(btnode* root) {
    if (root == NULL) return;
    for (int i = 0; i < root->nOChildren; i++) {
        freeTree(root->children[i]);
    }
    free(root);
    
}

int btree_search(btnode* root, int key) {
    int pos = binarySearchPos(root, key);
    if (root->keys[pos] == key) {
        printf("Found %d\n", key);
        return 0;
    }
    else {
        if (root->nOChildren) {
            return btree_search(root->children[pos], key);
        }
        else {
            printf("No key %d in the tree\n", key);
            return 1;
        }
    }

}

void moveKeyAndVal(btnode* from, int fromIdx, btnode* to, int toIdx) {
    to->keys[toIdx] = from->keys[fromIdx];
    to->values[toIdx] = from->values[fromIdx];
}