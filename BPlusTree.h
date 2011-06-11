#ifndef B_PLUS_TREE_H
#define B_PLUS_TREE_H

#include <exception>
#include <iostream>

#define B_TREE_FILE_HEADER "B_TREE_CREATED_BY_SUNZHENG"
#define B_TREE_FILE_HEADER_LEN strlen(B_TREE_FILE_HEADER)

class FileManager;

class BPlusTreeException : public std::exception{
    public:
        BPlusTreeException(int errNo);
        BPlusTreeException(const BPlusTreeException &e);

        const char *msg() const throw();
        
        static const int ERR_FILE_NOT_OPEN = 0;

    private:
        int _errNo;
};

class BPlusTree{
    public: 
        BPlusTree(FileManager *fm, int indexType, int indexLen);
        ~BPlusTree();
        
        bool insert(void *data, int posPage, int posSlot);
        void print() const;
        std::pair <int, int> query(void *data);
        bool remove(void *data);

        static const int IDX_TYPE_INT = 0;
        static const int IDX_TYPE_STRING = 1; 

    private:
        struct BPlusTreeBlock{
            int position;
            int type;
            union {
                struct {
                    int size;
                    int next;
                    int *posPage;
                    int *posSlot;
                    void *value;
                } leaf;
                struct {
                    int size;
                    int *child;
                    void *value;
                } nonleaf;
                struct {
                    int next;
                } empty;
            } data;
        };

        static const int TREE_NODE_TYPE_EMPTY = 0;
        static const int TREE_NODE_TYPE_LEAF = 1;
        static const int TREE_NODE_TYPE_NONLEAF = 2;

        FileManager *_fm;
        BPlusTreeBlock *_rootBlock;
        int _idxType;
        int _idxLen;
        int _blkSize;
        int _nonLeafDataCount;
        int _leafDataCount;
        int _emptyNode;
        
        void addEmptyBlock_p(int position);
        void addToLeaf_p(void *data, int posPage, int posSlot, BPlusTreeBlock *block, int loc = -1);
        void addToNonLeaf_p(BPlusTreeBlock *blockAdd, BPlusTreeBlock *block, int loc = -1);
        int calcBlockPosition_p(void *data, BPlusTreeBlock *block, int *equals = 0);
        void clearBlock_p(BPlusTreeBlock *&block) const;
        int emptyBlockPosition_p();
        bool insert_p(BPlusTreeBlock *block, void *data, int posPage, int posSlot);
        void mergeLeaf_p(BPlusTreeBlock *block, BPlusTreeBlock *nextBlock);
        void mergeNonLeaf_p(BPlusTreeBlock *block, BPlusTreeBlock *nextBlock);
        BPlusTreeBlock *newBlock_p(int position, int type);
        void print_p(BPlusTreeBlock *block) const;
        BPlusTreeBlock *readBlock_p(int position) const;
        bool remove_p(BPlusTreeBlock *block, void *data);
        void removeFromLeaf_p(BPlusTreeBlock *block, int loc);
        void removeFromNonLeaf_p(BPlusTreeBlock *block, int loc);
        BPlusTreeBlock *splitLeaf_p(BPlusTreeBlock *block);
        BPlusTreeBlock *splitNonLeaf_p(BPlusTreeBlock *block);
        void writeBlock_p(BPlusTreeBlock *block);
        void writeHeader_p();
};

#endif
