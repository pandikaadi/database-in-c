# database-in-c
B+ Tree Key-Value Store
A lightweight key-value store built from scratch in C, featuring a B+ tree for indexing. Designed for high-performance storage and retrieval, with support for dynamic page management and efficient key lookups.

Features
✅ B+ Tree-based indexing for efficient searches
✅ Supports insertions, deletions, and key promotions
✅ Disk persistence using file-backed storage
✅ Page splitting and merging for balanced tree operations
✅ Implements a simple database header for metadata management

🚧 Upcoming Features

Write-Ahead Logging (WAL) for durability and crash recovery

How It Works
This database stores key-value pairs using a B+ tree structure, with each node stored on disk. When inserting data:

1. Find the correct leaf page
2. Insert the key-value pair, shifting values if necessary
3. Split the node if it exceeds the maximum capacity
4. Promote a key to the parent node to maintain balance
