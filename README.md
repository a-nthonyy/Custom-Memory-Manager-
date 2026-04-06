# Custom Memory Manager

A custom memory management system built in C++ that handles initializing, tracking, allocating, and deallocating sections of memory. Developed as part of COP4600 (Operating Systems) at the University of Florida.

## Disclaimer

These projects are mine and mine alone. If any form of plagiarism is enacted based on my original work, I will not be afraid to take action in order to defend my self-image and personal integrity.

---

## Overview

This project implements a memory manager from scratch using a doubly linked list to track allocated and free memory regions. The manager supports dynamic allocation and deallocation with automatic hole merging (coalescing adjacent free blocks), and provides two pluggable allocation strategies: Best Fit and Worst Fit. Memory is acquired at the system level using POSIX `mmap` rather than `new` or `malloc`, and the manager exposes its internal state through a hole list, bitmap, and memory map dump for debugging and verification.

## Build

Running `make` compiles the source into a static library:

```bash
make
```

This produces `MemoryManager.o` and packages it into `libMemoryManager.a`.

To clean build artifacts:

```bash
make clean
```

## Testing

`CommandLineTest.cpp` contains a test suite that validates allocation, deallocation, hole merging, bitmap/list output, memory map dumps, getters, and direct memory read/write. To build and run the tests:

```bash
make
g++ -std=c++17 -o test CommandLineTest.cpp -L ./MemoryManager -lMemoryManager
./test
```

To check for memory leaks and errors:

```bash
valgrind --leak-check=full ./test
```

## Tools & Technologies

- **Language:** C++17
- **Memory:** POSIX `mmap` / `munmap`
- **File I/O:** POSIX `open`, `write`, `close`
- **Build:** GNU Make, `ar` (static library)
- **Testing:** Valgrind (memory leak detection)
