all: libMemoryManager.a

MemoryManager.o: MemoryManager.cpp MemoryManager.h
	g++ -std=c++17 -g -c MemoryManager.cpp -o MemoryManager.o

libMemoryManager.a: MemoryManager.o
	ar cr libMemoryManager.a MemoryManager.o

clean:
	rm -f MemoryManager.o libMemoryManager.a
