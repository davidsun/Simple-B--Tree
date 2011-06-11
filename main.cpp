#include "FileManager.h"
#include "BPlusTree.h"

#include <stdio.h>
#include <stdlib.h>

int main(void){
    FileManager *fm = new FileManager(); 
    try {
        fm -> openFile("B+Tree.index");
    }  catch (FileManagerException e){
        try {
            fm -> createFile("B+Tree.index", 4096);
        }  catch (FileManagerException e){
            printf("ERROR: %s\n", e.msg());
        }
    }
    try {
        BPlusTree *tree = new BPlusTree(fm, BPlusTree::IDX_TYPE_STRING, 20);
        for (int i = 0; i < 50; i ++){
            char c[20];
            for (int j = 0; j < 20; j ++) c[j] = rand() % 40 + 65;
            //std::pair <int, int> ret = tree -> query(c);
            //printf("%d %d\n", ret.first, ret.second);
            tree -> insert(c, i, 3);
            //tree -> print();
            //printf("\n\n");
            //tree -> remove(c);
            //tree -> print();
            //printf("\n\n");
            //if (tree -> insert(&i, 2, 3)) printf("NO\n");
            //printf("%d\n", tree -> insert(c, 2, 3));
            //tree -> print();
            //printf("\n\n");
        }
        /*for (int i = 0; i < 500000; i ++){
            //tree -> print();
            tree -> remove(&i);
            //tree -> print();
            //printf("\n");
        }
        for (int i = 500000 - 1; i >= 0; i --){
            tree -> insert(&i, i, 3);
        }
        for (int i = 0; i < 500000; i ++){
            //tree -> print();
            tree -> remove(&i);
            //tree -> print();
            //printf("\n");
        }*/
        delete tree;
    }  catch (BPlusTreeException e){
        printf("ERROR: %s\n", e.msg());
    }
    delete fm;
    return 0;
}
