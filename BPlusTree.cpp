/*
 * B+ Tree, implement by Sunzheng
 */

#include "BPlusTree.h"
#include "FileManager.h"

#include <string.h>
#include <stdio.h>

BPlusTreeException::BPlusTreeException(int errNo) : _errNo(errNo){
}

BPlusTreeException::BPlusTreeException(const BPlusTreeException &e) : _errNo(e._errNo){
}

const char *BPlusTreeException::msg() const throw(){
    switch (_errNo){
        case ERR_FILE_NOT_OPEN : 
            return "file not open";
        default :
            return "unknown error";
    }
}

BPlusTree::BPlusTree(FileManager *fm, int indexType, int indexLen) : _fm(fm), _idxType(indexType), _idxLen(indexLen){
    if (!fm -> isOpen()) throw BPlusTreeException(BPlusTreeException::ERR_FILE_NOT_OPEN);
    if (_idxType == IDX_TYPE_INT) _idxLen = sizeof(int);
    _blkSize = _fm -> blockSize();
    _nonLeafDataCount = (_blkSize - sizeof(int) * 3) / (sizeof(int) + _idxLen);
    _leafDataCount = (_blkSize - sizeof(int) * 3) / (_idxLen + sizeof(int) * 2);

    char *s = fm -> readString(_blkSize, B_TREE_FILE_HEADER_LEN);
    if (strcmp(s, B_TREE_FILE_HEADER)){
        /* Following function call (writeHeader_p) cannot be removed, for header is needed */
        _rootBlock = 0;
        writeHeader_p();

        _emptyNode = 0; 
        _rootBlock = newBlock_p(emptyBlockPosition_p(), TREE_NODE_TYPE_LEAF);
        writeBlock_p(_rootBlock);
        writeHeader_p();
    }  else {
        _rootBlock = readBlock_p(fm -> readInt(_blkSize + B_TREE_FILE_HEADER_LEN));
        _emptyNode = fm -> readInt(_blkSize + B_TREE_FILE_HEADER_LEN + sizeof(int));
    }
    delete[] s;
}

BPlusTree::~BPlusTree(){
    clearBlock_p(_rootBlock);
}

bool BPlusTree::insert(void *data, int posPage, int posSlot){
    BPlusTreeBlock *block = 0;
    bool ret = insert_p(_rootBlock, data, posPage, posSlot);
    BPlusTreeBlock *newBlock = 0;
    if (_rootBlock -> type == TREE_NODE_TYPE_LEAF && _rootBlock -> data.leaf.size > _leafDataCount) newBlock = splitLeaf_p(_rootBlock);
    else if (_rootBlock -> type == TREE_NODE_TYPE_NONLEAF && _rootBlock -> data.nonleaf.size > _nonLeafDataCount) newBlock = splitNonLeaf_p(_rootBlock);
    if (newBlock){
        int pos = emptyBlockPosition_p();
        BPlusTreeBlock *newRoot = newBlock_p(pos, TREE_NODE_TYPE_NONLEAF);
        newRoot -> data.nonleaf.child[0] = _rootBlock -> position;
        addToNonLeaf_p(newBlock, newRoot);
        writeBlock_p(newRoot);
        writeBlock_p(newBlock);
        writeBlock_p(_rootBlock);
        clearBlock_p(newBlock);
        clearBlock_p(_rootBlock);
        _rootBlock = newRoot;
        writeHeader_p();
    }
    return ret;
}

void BPlusTree::print() const{
    print_p(_rootBlock);
}

std::pair <int, int> BPlusTree::query(void *data){
    std::pair <int, int> ret(-1, -1);
    BPlusTreeBlock *b = _rootBlock;
    while (b -> type != TREE_NODE_TYPE_LEAF){
        BPlusTreeBlock *next = readBlock_p(b -> data.nonleaf.child[calcBlockPosition_p(data, b)]);
        if (b != _rootBlock) clearBlock_p(b);
        b = next;
    }
    int equals;
    int loc = calcBlockPosition_p(data, b, &equals);
    if (equals) ret = std::make_pair(b -> data.leaf.posPage[loc - 1], b -> data.leaf.posSlot[loc - 1]);
    if (b != _rootBlock) clearBlock_p(b);
    return ret;
}

bool BPlusTree::remove(void *data){
    bool ret = remove_p(_rootBlock, data);
    if (ret && _rootBlock -> type == TREE_NODE_TYPE_NONLEAF && _rootBlock -> data.nonleaf.size == 0){
        BPlusTreeBlock *newBlock = readBlock_p(_rootBlock -> data.nonleaf.child[0]);
        addEmptyBlock_p(_rootBlock -> position);
        clearBlock_p(_rootBlock);
        _rootBlock = newBlock;
        writeHeader_p();
    }
    return ret;
}

void BPlusTree::clearBlock_p(BPlusTreeBlock *&block) const{
    if (!block) return;
    if (block -> type == TREE_NODE_TYPE_EMPTY){
    }  else if (block -> type == TREE_NODE_TYPE_LEAF){
        delete []block -> data.leaf.posPage;
        delete []block -> data.leaf.posSlot;
        delete [](char *)block -> data.leaf.value;
    }  else if (block -> type == TREE_NODE_TYPE_NONLEAF){
        delete []block -> data.nonleaf.child;
        delete [](char *)block -> data.nonleaf.value;
    }
    delete block;
    block = 0;
}

void BPlusTree::addEmptyBlock_p(int position){
    BPlusTreeBlock *block = newBlock_p(position, TREE_NODE_TYPE_EMPTY);
    block -> data.empty.next = _emptyNode;
    _emptyNode = position;
    writeBlock_p(block);
    writeHeader_p();
    clearBlock_p(block);
}

void BPlusTree::addToLeaf_p(void *data, int posPage, int posSlot, BPlusTreeBlock *block, int loc){
    if (loc == -1) loc = calcBlockPosition_p(data, block);
    char *arr = (char *)block -> data.leaf.value;
    int size = block -> data.leaf.size ++;
    memmove(arr + (loc + 1) * _idxLen, arr + loc * _idxLen, (size - loc) * _idxLen); 
    memcpy(arr + loc * _idxLen, data, _idxLen);
    memmove(block -> data.leaf.posPage + loc + 1, block -> data.leaf.posPage + loc, (size - loc) * sizeof(int));
    block -> data.leaf.posPage[loc] = posPage;
    memmove(block -> data.leaf.posSlot + loc + 1, block -> data.leaf.posSlot + loc, (size - loc) * sizeof(int));
    block -> data.leaf.posSlot[loc] = posSlot;
}

void BPlusTree::addToNonLeaf_p(BPlusTreeBlock *blockAdd, BPlusTreeBlock *block, int loc){
    BPlusTreeBlock *b = blockAdd;
    while (b -> type != TREE_NODE_TYPE_LEAF){
        BPlusTreeBlock *newBlock = readBlock_p(b -> data.nonleaf.child[0]);
        if (b != blockAdd) clearBlock_p(b);
        b = newBlock;
    }
    if (loc == -1) loc = calcBlockPosition_p(b -> data.leaf.value, block);
    char *arr = (char *)block -> data.nonleaf.value;
    int size = block -> data.nonleaf.size ++;
    memmove(arr + (loc + 1) * _idxLen, arr + loc * _idxLen, (size - loc) * _idxLen); 
    memcpy(arr + loc * _idxLen, b -> data.leaf.value, _idxLen);
    memmove(block -> data.nonleaf.child + loc + 2, block -> data.nonleaf.child + loc + 1, (size - loc) * sizeof(int));
    block -> data.nonleaf.child[loc + 1] = blockAdd -> position;
    if (b != blockAdd) clearBlock_p(b);
}

int BPlusTree::calcBlockPosition_p(void *data, BPlusTreeBlock *block, int *equals){
    int size;
    void *value;
    if (block -> type == TREE_NODE_TYPE_NONLEAF){
        size = block -> data.nonleaf.size;
        value = block -> data.nonleaf.value;
    }  else if (block -> type == TREE_NODE_TYPE_LEAF){
        size = block -> data.leaf.size;
        value = block -> data.leaf.value;
    }
    int l = 0, r = size - 1;
    if (_idxType == IDX_TYPE_STRING){
        char *x = (char *)data;
        char *arr = (char *)value;
        while (l < r){
            int mid = (l + r) >> 1;
            int c = strncmp(x, arr + mid * _idxLen, _idxLen);
            if (c > 0) r = mid - 1;
            else if (c == 0) l = r = mid;
            else l = mid + 1;
        }
        //printf("%s %s %d %d %d\n", x, arr + l * _idxLen, strncmp(x, arr + l * _idxLen, _idxLen), l, r);
        if (equals) *equals = (l < size && strncmp(x, arr + l * _idxLen, _idxLen) == 0);
        while (l < size && strncmp(x, arr + l * _idxLen, _idxLen) <= 0) l ++;
        while (l > 0 && strncmp(x, arr + (l - 1) * _idxLen, _idxLen) > 0) l --;
    }  else if (_idxType == IDX_TYPE_INT){
        int x = *((int *)data);
        int *arr = (int *)value;
        while (l < r){
            int mid = (l + r) >> 1;
            if (arr[mid] < x) l = mid + 1;
            else if (arr[mid] > x) r = mid - 1;
            else l = r = mid;
        }
        if (equals) *equals = (l < size && arr[l] == x);
        while (l < size && arr[l] <= x) l ++;
        while (l > 0 && arr[l - 1] > x) l --;
        //for (int i = 0; i < size; i ++) printf("%d ", arr[i]);
        //printf("(%d)\n", size);
    }
    return l;
}

int BPlusTree::emptyBlockPosition_p(){
    if (_emptyNode == 0){
        char block[_blkSize];
        memset(block, 0, _blkSize);
        return _fm -> writeNewBlock(block) / _blkSize;
    }
    int ret = _emptyNode;
    BPlusTreeBlock *block = readBlock_p(ret);
    _emptyNode = block -> data.empty.next;
    clearBlock_p(block);
    writeHeader_p();
    return ret;
}

bool BPlusTree::insert_p(BPlusTreeBlock *block, void *data, int posPage, int posSlot){
    bool ret;
    if (block -> type == TREE_NODE_TYPE_LEAF){
        int equals;
        int loc = calcBlockPosition_p(data, block, &equals);
        if (equals) ret = 0;
        else {
            addToLeaf_p(data, posPage, posSlot, block, loc);
            if (block -> data.leaf.size <= _leafDataCount) writeBlock_p(block);
            ret = 1;
        }
    }  else if (block -> type == TREE_NODE_TYPE_NONLEAF){
        int loc = calcBlockPosition_p(data, block);
        BPlusTreeBlock *child = readBlock_p(block -> data.nonleaf.child[loc]);
        ret = insert_p(child, data, posPage, posSlot);
        BPlusTreeBlock *newBlock = 0;
        if (child -> type == TREE_NODE_TYPE_NONLEAF && child -> data.nonleaf.size > _nonLeafDataCount) newBlock = splitNonLeaf_p(child);
        else if (child -> type == TREE_NODE_TYPE_LEAF && child -> data.leaf.size > _leafDataCount) newBlock = splitLeaf_p(child);
        if (newBlock){
            writeBlock_p(child);
            writeBlock_p(newBlock);
            addToNonLeaf_p(newBlock, block, loc);
            if (block -> data.nonleaf.size <= _nonLeafDataCount) writeBlock_p(block);
            clearBlock_p(newBlock);
        }
        clearBlock_p(child);
    }
    return ret;
}

void BPlusTree::mergeLeaf_p(BPlusTreeBlock *block, BPlusTreeBlock *nextBlock){
    int lSize = block -> data.leaf.size, rSize = nextBlock -> data.leaf.size;
    block -> data.leaf.next = nextBlock -> data.leaf.next;
    memcpy(block -> data.leaf.posPage + lSize, nextBlock -> data.leaf.posPage, sizeof(int) * rSize);
    memcpy(block -> data.leaf.posSlot + lSize, nextBlock -> data.leaf.posSlot, sizeof(int) * rSize);
    memcpy(((char *)block -> data.leaf.value) + _idxLen * lSize, nextBlock -> data.leaf.value, _idxLen * rSize);
    block -> data.leaf.size = lSize + rSize;
}

void BPlusTree::mergeNonLeaf_p(BPlusTreeBlock *block, BPlusTreeBlock *nextBlock){
    BPlusTreeBlock *b = nextBlock;
    while (b -> type != TREE_NODE_TYPE_LEAF){
        BPlusTreeBlock *nb = readBlock_p(b -> data.nonleaf.child[0]);
        if (b != nextBlock) clearBlock_p(b);
        b = nb;
    }
    int lSize = block -> data.nonleaf.size, rSize = nextBlock -> data.nonleaf.size;
    memcpy(block -> data.nonleaf.child + lSize + 1, nextBlock -> data.nonleaf.child, sizeof(int) * (rSize + 1));
    memcpy(((char *)block -> data.nonleaf.value) + _idxLen * (lSize + 1), nextBlock -> data.nonleaf.value, _idxLen * rSize);
    memcpy(((char *)block -> data.nonleaf.value) + _idxLen * lSize, b -> data.leaf.value, _idxLen);
    block -> data.nonleaf.size = lSize + rSize + 1;
    clearBlock_p(b);
}

BPlusTree::BPlusTreeBlock *BPlusTree::newBlock_p(int position, int type){
    BPlusTreeBlock *ret = new BPlusTreeBlock;
    ret -> position = position;
    ret -> type = type;
    if (ret -> type == TREE_NODE_TYPE_EMPTY){
        ret -> data.empty.next = 0;
    }  else if (ret -> type == TREE_NODE_TYPE_LEAF){
        ret -> data.leaf.size = 0;
        ret -> data.leaf.next = 0;
        ret -> data.leaf.posPage = new int[_leafDataCount + 1];
        ret -> data.leaf.posSlot = new int[_leafDataCount + 1];
        ret -> data.leaf.value = (void *)new char[_leafDataCount * _idxLen + _idxLen];
    }  else {
        ret -> data.nonleaf.size = 0;
        ret -> data.nonleaf.child = new int[_nonLeafDataCount + 2];
        ret -> data.nonleaf.value = (void *)new char[_nonLeafDataCount * _idxLen + _idxLen];
    }
    return ret;
}

void BPlusTree::print_p(BPlusTreeBlock *block) const{
    void *data;
    int size;
    if (block -> type == TREE_NODE_TYPE_LEAF){
        data = block -> data.leaf.value;
        size = block -> data.leaf.size;
    }  else {
        data = block -> data.nonleaf.value;
        size = block -> data.nonleaf.size;
    }
    if (_idxType == IDX_TYPE_INT){
        int *arr = (int *)data;
        for (int i = 0; i < size; i ++){
            if (block -> type == TREE_NODE_TYPE_NONLEAF){
                printf("(");
                BPlusTreeBlock *child = readBlock_p(block -> data.nonleaf.child[i]);
                print_p(child);
                clearBlock_p(child);
                printf(")");
            }
            printf(" %d", arr[i]);
            if (block -> type == TREE_NODE_TYPE_LEAF)
                printf(",%d,%d", block -> data.leaf.posPage[i], block -> data.leaf.posSlot[i]);
            putchar(' ');
        }
        if (block -> type == TREE_NODE_TYPE_NONLEAF){
            printf("(");
            BPlusTreeBlock *child = readBlock_p(block -> data.nonleaf.child[size]);
            print_p(child);
            clearBlock_p(child);
            printf(")");
        }
    }  else {
        char *arr = (char *)data;
        for (int i = 0; i < size; i ++){
            if (block -> type == TREE_NODE_TYPE_NONLEAF){
                printf("(");
                BPlusTreeBlock *child = readBlock_p(block -> data.nonleaf.child[i]);
                print_p(child);
                clearBlock_p(child);
                printf(")");
            }
            putchar(' ');
            for (int j = 0; j < _idxLen; j ++) 
                putchar(arr[j + i * _idxLen]);
            if (block -> type == TREE_NODE_TYPE_LEAF)
                printf(",%d,%d", block -> data.leaf.posPage[i], block -> data.leaf.posSlot[i]);
            putchar(' ');
        }
        if (block -> type == TREE_NODE_TYPE_NONLEAF){
            printf("(");
            BPlusTreeBlock *child = readBlock_p(block -> data.nonleaf.child[size]);
            print_p(child);
            clearBlock_p(child);
            printf(")");
        }
    }
}

bool BPlusTree::remove_p(BPlusTreeBlock *block, void *data){
    bool ret;
    if (block -> type == TREE_NODE_TYPE_LEAF){
        int equals;
        int loc = calcBlockPosition_p(data, block, &equals) - 1;
        if (!equals) ret = 0;
        else {
            removeFromLeaf_p(block, loc);
            writeBlock_p(block);
            ret = 1;
        }
    }  else if (block -> type == TREE_NODE_TYPE_NONLEAF){
        int loc = calcBlockPosition_p(data, block);
        BPlusTreeBlock *child = readBlock_p(block -> data.nonleaf.child[loc]);
        ret = remove_p(child, data);
        if (ret){
            BPlusTreeBlock *nextChild = 0, *lastChild = 0;
            if (loc > 0) lastChild = readBlock_p(block -> data.nonleaf.child[loc - 1]);
            if (loc < block -> data.nonleaf.size) nextChild = readBlock_p(block -> data.nonleaf.child[loc + 1]);
            if (lastChild && ((child -> type == TREE_NODE_TYPE_NONLEAF && lastChild -> data.nonleaf.size + child -> data.nonleaf.size + 1 <= _nonLeafDataCount)
                    || (child -> type == TREE_NODE_TYPE_LEAF && lastChild -> data.leaf.size + child -> data.leaf.size <= _leafDataCount))){
                if (child -> type == TREE_NODE_TYPE_LEAF) mergeLeaf_p(lastChild, child);
                else mergeNonLeaf_p(lastChild, child);
                addEmptyBlock_p(child -> position);
                removeFromNonLeaf_p(block, loc);
                writeBlock_p(lastChild);
                writeBlock_p(block);
            }  else if (nextChild && ((child -> type == TREE_NODE_TYPE_NONLEAF && 
                            nextChild -> data.nonleaf.size + child -> data.nonleaf.size + 1 <= _nonLeafDataCount) || (child -> type == TREE_NODE_TYPE_LEAF &&
                            nextChild -> data.leaf.size + child -> data.leaf.size <= _leafDataCount))){
                if (child -> type == TREE_NODE_TYPE_LEAF) mergeLeaf_p(child, nextChild);
                else mergeNonLeaf_p(child, nextChild); 
                addEmptyBlock_p(nextChild -> position);
                removeFromNonLeaf_p(block, loc + 1);
                writeBlock_p(child);
                writeBlock_p(block);
            }
            if (nextChild) clearBlock_p(nextChild);
            if (lastChild) clearBlock_p(lastChild);
        }
        clearBlock_p(child);
    }
    return ret;
}

void BPlusTree::removeFromLeaf_p(BPlusTreeBlock *block, int loc){
    int size = block -> data.leaf.size --;
    memmove(block -> data.leaf.posPage + loc, block -> data.leaf.posPage + loc + 1, (size - loc - 1) * sizeof(int));
    memmove(block -> data.leaf.posSlot + loc, block -> data.leaf.posSlot + loc + 1, (size - loc - 1) * sizeof(int));
    memmove(((char *)block -> data.leaf.value) + _idxLen * loc, ((char *)block -> data.leaf.value) + _idxLen * (loc + 1), (size - loc - 1) * _idxLen);
}

/* Remove the child and the value BEFORE it */
void BPlusTree::removeFromNonLeaf_p(BPlusTreeBlock *block, int loc){
    int size = block -> data.nonleaf.size --;
    memmove(block -> data.nonleaf.child + loc, block -> data.nonleaf.child + loc + 1, (size - loc) * sizeof(int));
    memmove(((char *)block -> data.nonleaf.value) + _idxLen * (loc - 1), ((char *)block -> data.nonleaf.value) + _idxLen * loc, (size - loc) * _idxLen);
}

BPlusTree::BPlusTreeBlock *BPlusTree::splitLeaf_p(BPlusTreeBlock *block){
    BPlusTreeBlock *ret = newBlock_p(emptyBlockPosition_p(), TREE_NODE_TYPE_LEAF);
    int lSize = block -> data.leaf.size / 2, rSize = block -> data.leaf.size - lSize;
    ret -> data.leaf.next = block -> data.leaf.next;
    block -> data.leaf.next = ret -> position;
    memcpy(ret -> data.leaf.posPage, block -> data.leaf.posPage + lSize, sizeof(int) * rSize);
    memcpy(ret -> data.leaf.posSlot, block -> data.leaf.posSlot + lSize, sizeof(int) * rSize);
    char *arr = (char *)block -> data.leaf.value;
    memcpy(ret -> data.leaf.value, arr + _idxLen * lSize, _idxLen * rSize);
    block -> data.leaf.size = lSize;
    ret -> data.leaf.size = rSize;
    return ret;
}

BPlusTree::BPlusTreeBlock *BPlusTree::splitNonLeaf_p(BPlusTreeBlock *block){
    BPlusTreeBlock *ret = newBlock_p(emptyBlockPosition_p(), TREE_NODE_TYPE_NONLEAF);
    int lSize = block -> data.nonleaf.size / 2, rSize = block -> data.nonleaf.size - lSize - 1;
    memcpy(ret -> data.nonleaf.child, block -> data.nonleaf.child + lSize + 1, sizeof(int) * (rSize + 1));
    char *arr = (char *)block -> data.nonleaf.value;
    memcpy(ret -> data.nonleaf.value, arr + _idxLen * (lSize + 1), _idxLen * rSize);
    block -> data.nonleaf.size = lSize;
    ret -> data.nonleaf.size = rSize;
    return ret;
}

BPlusTree::BPlusTreeBlock *BPlusTree::readBlock_p(int position) const{
    char *d = _fm -> readBlock(position * _blkSize);
    char *data = d;
    BPlusTreeBlock *ret = new BPlusTreeBlock;
    ret -> position = position;
    memcpy(&ret -> type, data, sizeof(int));
    data += sizeof(int);
    if (ret -> type == TREE_NODE_TYPE_EMPTY){
        memcpy(&ret -> data.empty.next, data, sizeof(int));
    }  else if (ret -> type == TREE_NODE_TYPE_LEAF){
        memcpy(&ret -> data.leaf.size, data, sizeof(int));
        data += sizeof(int);

        memcpy(&ret -> data.leaf.next, data, sizeof(int));
        data += sizeof(int);

        ret -> data.leaf.posPage = new int[_leafDataCount + 1];
        memcpy(ret -> data.leaf.posPage, data, sizeof(int) * _leafDataCount);
        data += sizeof(int) * _leafDataCount;

        ret -> data.leaf.posSlot = new int[_leafDataCount + 1];
        memcpy(ret -> data.leaf.posSlot, data, sizeof(int) * _leafDataCount);
        data += sizeof(int) * _leafDataCount;

        ret -> data.leaf.value = (void *)new char[_leafDataCount * _idxLen + _idxLen];
        memcpy(ret -> data.leaf.value, data, _leafDataCount * _idxLen);
        //printf("%d %d\n", data - d + _leafDataCount * _idxLen, _blkSize);
    }  else if (ret -> type == TREE_NODE_TYPE_NONLEAF){
        memcpy(&ret -> data.nonleaf.size, data, sizeof(int));
        data += sizeof(int);

        ret -> data.nonleaf.child = new int[_nonLeafDataCount + 2];
        memcpy(ret -> data.nonleaf.child, data, sizeof(int) * (_nonLeafDataCount + 1));
        data += sizeof(int) * (_nonLeafDataCount + 1);

        ret -> data.nonleaf.value = (void *)new char[_nonLeafDataCount * _idxLen + _idxLen];
        memcpy(ret -> data.nonleaf.value, data, _nonLeafDataCount * _idxLen);
        //printf("%d %d\n", data - d + _nonLeafDataCount * _idxLen, _blkSize);
    }
    delete []d;
    return ret;
}

void BPlusTree::writeBlock_p(BPlusTreeBlock *block){
    char d[_blkSize];
    char *data = d;
    memcpy(data, &block -> type, sizeof(int));
    data += sizeof(int);
    if (block -> type == TREE_NODE_TYPE_EMPTY){
        memcpy(data, &block -> data.empty.next, sizeof(int));
    }  else if (block -> type == TREE_NODE_TYPE_LEAF){
        memcpy(data, &block -> data.leaf.size, sizeof(int));
        data += sizeof(int);

        memcpy(data, &block -> data.leaf.next, sizeof(int));
        data += sizeof(int);

        memcpy(data, block -> data.leaf.posPage, sizeof(int) * _leafDataCount);
        data += sizeof(int) * _leafDataCount;

        memcpy(data, block -> data.leaf.posSlot, sizeof(int) * _leafDataCount);
        data += sizeof(int) * _leafDataCount;

        memcpy(data, block -> data.leaf.value, _leafDataCount * _idxLen);
    }  else if (block -> type == TREE_NODE_TYPE_NONLEAF){
        memcpy(data, &block -> data.nonleaf.size, sizeof(int));
        data += sizeof(int);

        memcpy(data, block -> data.nonleaf.child, sizeof(int) * (_nonLeafDataCount + 1));
        data += sizeof(int) * (_nonLeafDataCount + 1);

        memcpy(data, block -> data.nonleaf.value, _nonLeafDataCount * _idxLen);
    }
    _fm -> writeBlock(block -> position * _blkSize, d);
}

void BPlusTree::writeHeader_p(){
    char block[_blkSize];
    memset(block, 0, _blkSize);
    memcpy(block, B_TREE_FILE_HEADER, B_TREE_FILE_HEADER_LEN);
    if (_rootBlock) memcpy(block + B_TREE_FILE_HEADER_LEN, &_rootBlock -> position, sizeof(int));
    memcpy(block + B_TREE_FILE_HEADER_LEN + sizeof(int), &_emptyNode, sizeof(int));
    _fm -> writeBlock(_blkSize, block);
}


