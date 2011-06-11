#include "FileManager.h"

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

FileManagerException::FileManagerException(int errNo) : exception(), _errNo(errNo){
}

FileManagerException::FileManagerException(const FileManagerException &e) : _errNo(e._errNo){
}

const char *FileManagerException::msg() const throw(){
    switch (_errNo){
        case ERR_FILE_NOT_OPEN :
            return "file not open";
        case ERR_INVALID_FILE :
            return "invalid file";
        case ERR_INVALID_FILE_NAME :
            return "invalid file name";
        default :
            return "unknown error";
    }
}

FileManager::FileManager() : _fd(0){
}

FileManager::~FileManager(){
    closeFile();
}

void FileManager::closeFile(){
    if (!_fd) return;
    close(_fd);
    _fd = 0;
}

int FileManager::blockSize() const{
    return _blockSize;
}

void FileManager::createFile(const char *fileName, int blockSize){
    if (_fd) closeFile();
    _fd = open(fileName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (_fd == -1){
        _fd = 0;
        throw FileManagerException(FileManagerException::ERR_INVALID_FILE_NAME);
    }
    _blockSize = blockSize;
    char block[blockSize];
    memset(block, 0, blockSize);
    memcpy(block, FILE_HEADER, FILE_HEADER_LEN); 
    memcpy(block + FILE_HEADER_LEN, &blockSize, sizeof(int));
    writeBlock(0, block);
}

bool FileManager::isOpen() const{
    return (_fd != 0);
}

char *FileManager::readBlock(int position){
    if (!_fd) throw FileManagerException(FileManagerException::ERR_FILE_NOT_OPEN);
    char *ret = new char[_blockSize + 1];
    lseek(_fd, position, SEEK_SET);
    read(_fd, ret, _blockSize);
    ret[_blockSize] = 0;
    return ret;
}

int FileManager::readInt(int position){
    if (!_fd) throw FileManagerException(FileManagerException::ERR_FILE_NOT_OPEN);
    int ret;
    lseek(_fd, position, SEEK_SET);
    read(_fd, &ret, sizeof(int));
    return ret;
}

long FileManager::readLong(int position){
    if (!_fd) throw FileManagerException(FileManagerException::ERR_FILE_NOT_OPEN);
    long ret;
    lseek(_fd, position, SEEK_SET);
    read(_fd, &ret, sizeof(long));
    return ret;
}

char *FileManager::readString(int position, int length){
    if (!_fd) throw FileManagerException(FileManagerException::ERR_FILE_NOT_OPEN);
    char *ret = new char[length + 1];
    lseek(_fd, position, SEEK_SET);
    read(_fd, ret, length);
    ret[length] = 0;
    return ret;
}

void FileManager::openFile(const char *fileName){
    if (_fd) closeFile();
    _fd = open(fileName, O_RDWR, S_IRUSR | S_IWUSR);
    if (_fd == -1){
        throw FileManagerException(FileManagerException::ERR_INVALID_FILE_NAME);
        _fd = 0;
    }
    char *s = readString(0, FILE_HEADER_LEN);
    if (!s){
        delete[] s;
        throw FileManagerException(FileManagerException::ERR_INVALID_FILE);
    }  else if (strcmp(s, FILE_HEADER)){
        delete[] s;
        throw FileManagerException(FileManagerException::ERR_INVALID_FILE);
    }
    delete[] s;
    _blockSize = readInt(FILE_HEADER_LEN);
}

int FileManager::writeBlock(int position, const char *data){
    if (!_fd) throw FileManagerException(FileManagerException::ERR_FILE_NOT_OPEN);
    int ret = lseek(_fd, position, SEEK_SET);
    write(_fd, data, _blockSize);
    return ret;
}

int FileManager::writeNewBlock(const char *data){
    if (!_fd) throw FileManagerException(FileManagerException::ERR_FILE_NOT_OPEN);
    int ret = lseek(_fd, 0, SEEK_END);
    write(_fd, data, _blockSize);
    return ret;
}
