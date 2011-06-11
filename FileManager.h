#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <exception>

#define FILE_HEADER "DB_FILE_CREATED_BY_SUNZHENG"
#define FILE_HEADER_LEN strlen(FILE_HEADER)

class FileManagerException : public std::exception{
    public:
        FileManagerException(int errNo);
        FileManagerException(const FileManagerException &e);

        const char *msg() const throw();
        
        static const int ERR_FILE_NOT_OPEN = 0;
        static const int ERR_INVALID_FILE = 1;
        static const int ERR_INVALID_FILE_NAME = 2;

    private:
        int _errNo;
};

class FileManager{
    public:
        FileManager();
        ~FileManager();
       
        int blockSize() const;
        void closeFile();
        void createFile(const char *fileName, int blockSize); 
        bool isOpen() const;
        void openFile(const char *fileName);
        char *readBlock(int position);
        int readInt(int position);
        long readLong(int position);
        char *readString(int position, int length);
        int writeBlock(int position, const char *data);
        int writeNewBlock(const char *data);

    private:
        int _fd;
        int _blockSize;
        char *_fileName;
};

#endif
