#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H
#include <functional>
#include <cstdint>
#include <cmath>
#include <string>

// POSIX INCLUDES!
#include <fcntl.h> 
#include <unistd.h>
#include <sys/mman.h>
#pragma once

using namespace std;

// Not part of memory manager class
int bestFit(int sizeInWords, void *list);
int worstFit(int sizeInWords, void *list);

// Linked List
class MemoryManager {
    // subject to change just using spec for now
    unsigned wordSize;
    function<int(int, void *)> allocator;
    void* memoryblock;
    size_t memory_limit;
    bool init;

public:
    struct Node{
        // contains info on segments
        unsigned int offset;
        unsigned int length;
        bool hole;
        Node* next;
        Node* prev;

        Node(unsigned int offset, unsigned int length, bool hole);
    };
    Node* head;
    Node* tail;

    MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator);
    void shutdown();
    ~MemoryManager(); // must use shutdown
    void initialize(size_t sizeInWords);
    void *allocate(size_t sizeInBytes);
    void free(void *address);
    void setAllocator(std::function<int(int, void *)> allocator);
    int dumpMemoryMap(char *filename);
    void *getList();
    void *getBitmap();
    unsigned getWordSize();
    void *getMemoryStart();
    unsigned getMemoryLimit();

    // Node Specific Functions
    void insertNode(int offset, int length);

};



#endif //MEMORYMANAGER_H
