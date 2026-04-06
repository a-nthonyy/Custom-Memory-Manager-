#include "MemoryManager.h"
using namespace std;

// --------------  ALLOCATOR FUNCTIONS ---------------------

// smallest hole that can fit word
int bestFit(int sizeInWords, void *list) { // returns word offset
    uint16_t* temp = static_cast<uint16_t*>(list);
    
    int best = 65536; 
    
    int listSize = 1 + (temp[0] * 2); 
    // [0] holds amt of holes, list contains only holes separated via [offset, length]
    
    int ret = -1;

    for (int i = 1; i < listSize; i += 2) {
        // i cannot be even because offset is at idx 1 then 3 then 5 etc
        int offset = temp[i];
        int length = temp[i+1];
        int currentFit = length - sizeInWords;

        if (currentFit < best && currentFit >= 0) {
            best = currentFit;
            ret = offset;
        }
    }
    return ret;
}

// largest hole that can fit word
int worstFit(int sizeInWords, void *list) { // returns word offset
    uint16_t* temp = static_cast<uint16_t*>(list);
    
    int worst = -1; 
    
    int listSize = 1 + (temp[0] * 2); 
    
    int ret = -1;

    for (int i = 1; i < listSize; i += 2) {
        // i cannot be even because offset is at idx 1 then 3 then 5 etc
        int offset = temp[i];
        int length = temp[i+1];
        int currentFit = length - sizeInWords;

        if (currentFit > worst && currentFit >= 0) {
            worst = currentFit;
            ret = offset;
        }
    }
    return ret;
}


void *MemoryManager::allocate(size_t sizeInBytes) {
    if (!init){
        return nullptr;
    } 

    int sizeInWords = 0;

    if (sizeInBytes % wordSize != 0) {
        sizeInWords = floor(sizeInBytes/wordSize) + 1;
    } else {
        // sizeInBytes can be cleanly divided by wordSize
        sizeInWords = sizeInBytes / wordSize;
    }

    void *list = getList();
    int offset = allocator(sizeInWords, list);
    delete[] static_cast<uint16_t*>(list); // AVOIDS MEM LEAKS!!!!!!!!!!!!!!!!!!! REMEMBER TO DELETE!!!!

    // If no memory available, returns nullptr
    if (offset == -1) {
        return nullptr;
    }

    insertNode(offset, sizeInWords);

    char *allocatedLocation = static_cast<char*>(memoryblock) + (offset * wordSize);
    return allocatedLocation;
}

// ----------------------- NODE FUNCTIONS ----------------------------------------
MemoryManager::Node::Node(unsigned int offset, unsigned int length, bool hole){
    this->offset = offset;
    this->length = length;
    this->hole = hole;
    this->next = nullptr;
    this->prev = nullptr;
}

// never called directly
void MemoryManager::insertNode(int offset, int length) {
    Node* block = head;

    while (block && block->offset != offset) {
        block = block->next;
    }

    //  If size invalid, returns
    if (!block || !block->hole) {
        return;
    } 
    
    int newHoleLength = block->length - length;

    // initiate block (same offset, new adjusted length)
    block->hole = false;
    block->length = length;

    Node* next_node = block->next;

    // if new hole is 0, then there is no new hole just temp
    if(newHoleLength > 0){
        Node* newHole = new Node(offset + length, newHoleLength, true);
        
        newHole->next = next_node;
        newHole->prev = block;
        block->next = newHole;

        if(next_node){
            next_node->prev = newHole;
        } else {
            tail = newHole;
        }
    }
}



// ---------------- MEMORY MANAGER / LINKED LIST CLASS FUNCTIONS ------------------------

// Constructor & Deconstructor
MemoryManager::MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator) {
    this->wordSize = wordSize;
    setAllocator(allocator);
    memoryblock = nullptr;
    memory_limit = 0;
    init = false;
}

void MemoryManager::shutdown() {
    Node* temp = head;
    while (temp) {
        Node* old_node = temp;
        temp = temp->next;
        delete old_node;
    }
    head = nullptr;
    tail = nullptr;
   //delete[] static_cast<uint8_t*>(memoryblock);
    munmap(memoryblock, memory_limit);
    memoryblock = nullptr;
    init = false;
}

MemoryManager::~MemoryManager() {
    shutdown();
}

void MemoryManager::initialize(size_t sizeInWords) {
    if (init) {
        shutdown();
    }

    if (sizeInWords > 65535) {
        return;
    }

    // a single contiguous array of dynamic memory of size
    // sizeInWords * wordSize bytes

    memory_limit = wordSize * sizeInWords;
    // memoryblock = new uint8_t[memory_limit];
    memoryblock = mmap(nullptr, memory_limit, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // basically one hole the size of the entire array
    head = new Node(0, sizeInWords, true);
    tail = head;
    init = true;
}


void *MemoryManager::getList(){
    // If no memory has been allocated, the function should return a NULL pointer
    if(!init){
        return nullptr;
    }

    int numHoles = 0;
    
    // COUNT HOLES!
    Node* temp = head;
    while (temp) {
        if (temp->hole)
            numHoles++;
        temp = temp->next;
    }

    // list format [holeAmount, hole1offset, hole1length, ... , holeioffset, holeilength]
    //                         | (2 indices per hole -> holeSize * 2)
    uint16_t* list = new uint16_t[1 + (numHoles*2)];
    list[0] = numHoles;

    temp = head;
    
    // SET LIST!
    int i = 1;
    while (temp) {
        if (temp->hole) {
            list[i] = temp->offset;
            list[i + 1] = temp->length;
            i += 2;
        }
        temp = temp->next;
    }
    
    return static_cast<void*>(list);
}

// Frees the memory block that starts at the given address within the memory manager so that it can be reused. 
void MemoryManager::free(void* address){
    if (!init || !address){
        return;
    }

    // *! char *allocatedLocation = static_cast<char*>(memoryblock) + (offset * wordSize);
    int offset = ((static_cast<char*>(address) - static_cast<char*>(memoryblock))) / wordSize;

    Node* curr = head;
    while(curr && curr->offset != offset){
        curr = curr->next;
    }

    if (!curr || curr->hole){
	// not found or already free
	return;
    }
    curr->hole = true;
    Node* next_node = curr->next;
    Node* prev_node = curr->prev;

    // merging next hole and current hole
    if(next_node && next_node->hole){
        curr->length += next_node->length; // offset does not change since curr is before
        
        // nothing points TO next_node
        curr->next = next_node->next;
        
        if(next_node->next){
            next_node->next->prev = curr;
        }
        
        // nothing pointed at FROM next_node (safe to delete)
        next_node->next = nullptr;
        next_node->prev = nullptr;

        // accounts for next node being last in memory
        if(next_node == tail){
            tail = curr;
        }

        delete next_node;
    }

    // merging prev hole and current hole
    if(prev_node && prev_node->hole){
        prev_node->length += curr->length;

        // nothing points TO curr
        prev_node->next = curr->next;

        if(curr->next){
            curr->next->prev = prev_node;
        }
        
        // nothing pointed at FROM curr (safe to delete)
        curr->next = nullptr;
        curr->prev = nullptr;

        // accounts for curr being last in memory
        if(curr == tail){
            tail = prev_node;
        }

        delete curr;
    }
}


int MemoryManager::dumpMemoryMap(char *filename){
    string writeList = "";
    
    Node* temp = head;

    while(temp){
        if(temp->hole){
            //"[0, 10] - " -> "[12, 2] - " -> "[20, 6] - "
            writeList += ("[" + to_string(temp->offset) + ", " + to_string(temp->length) + "] - "); 
        }
        temp = temp->next;
    }
    
    if(!writeList.empty()){
        // "[0, 10] - [12, 2] - [20, 6]"
        writeList.erase(writeList.length() - 3); // removes hanging " - "
    }

    // UNCOMMENT IN REPTILIAN:
    int file = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if(file == -1){
        return file;
    } else {
        write(file, writeList.c_str(), writeList.length());
        close(file);
        return 0;
    }
}

void *MemoryManager::getBitmap(){
    if (!init){
        return nullptr;
    } 

    Node* temp = head;
    int totalWords = memory_limit / wordSize;
    int totalBits = totalWords;

    while (totalBits % 8 != 0) {
        totalBits++;
    }

    // create array of bits, 0 initialized. one element per word.
    uint8_t* bitArray = new uint8_t[totalBits]();

    while (temp) {
        if (!temp->hole) {
            for (unsigned int i = temp->offset; i < temp->offset + temp->length; i++) {
                bitArray[i] = 1;
            }
        }
        temp = temp->next;
    }

    // pack every 8 bits into one byte using reversed powers
    int bitmapSize = totalBits / 8;
    int powers[] = {1, 2, 4, 8, 16, 32, 64, 128};

    // +2 for the 2 extra bytes
    uint8_t* result = new uint8_t[bitmapSize + 2]();

    // size prefix in little-endian
    result[0] = bitmapSize % 256;
    result[1] = bitmapSize / 256;

    // mirroring step via packing and least significant bit
    for (int i = 0; i < bitmapSize; i++) {
        int val = 0;
        for (int j = 0; j < 8; j++) {
            val += bitArray[i * 8 + j] * powers[j];
        }
        result[i + 2] = val;
    }

    delete[] bitArray;

    return static_cast<void*>(result);
}




// Getters and Setters
unsigned MemoryManager::getWordSize() {
    return wordSize;
}

void *MemoryManager::getMemoryStart() {
    return memoryblock;
}

unsigned MemoryManager::getMemoryLimit() {
    return memory_limit;
}

void MemoryManager::setAllocator(std::function<int(int, void *)> allocator) {
    this->allocator = allocator;
}
