# VectoC: Minimalist Vector Database & Semantic Search Engine

**VectoC** is a lightweight vector database developed in **C**, designed for high-performance semantic search. It combines the power of low-level systems programming (C) for storage and distance calculations with the flexibility of AI (Python) for embedding generation.

---

##  Technical Highlights

*   **K-NN Vector Search**: Implementation of a similarity search based on **Euclidean distance** ($L2$ norm): 
    $$d(p, q) = \sqrt{\sum_{i=1}^{n} (p_i - q_i)^2}$$
*   **Hybrid Architecture**: A high-performance **C** storage engine (Core) driven by a **Python** Natural Language Processing interface (Intelligence).
*   **Robust Persistence**: Direct binary storage management via POSIX system calls (`open`, `lseek`, `fsync`) ensuring data integrity even in the event of a crash.
*   **Custom TCP Protocol**: Multi-client server handling TCP packet fragmentation for the transmission of high-dimensional vectors (384-d).

---

##  System Architecture

The project utilizes a modern **decoupled** approach, similar to production-grade vector databases like Milvus or Pinecone:

1.  **C Server **: Manages binary storage, linear file scanning, and intensive mathematical computations.
2.  **Python Script **: Uses `sentence-transformers` (model `all-MiniLM-L6-v2`) to transform raw text into 384-dimensional vectors.
3.  **Metadata Mapper**: A JSON-based mapping system ensures correspondence between the C database IDs and the original text strings.

---

##  Supported Commands

### C Client (Low-Level)
- `insert <id> <vector...>`: Inserts a 384-dimensional vector.
- `search <vector...>`: Searches for the **Top-3** nearest vectors.
- `select`: Lists all database records.
- `delete <id>`: Logical deletion (Soft delete).

### Python Client (Semantic Search)
The `main_ai.py` script allows natural language interaction with the database:
- `insert_text("My sentence")`: Handles automatic encoding and insertion.
- `search_text("My query")`: Performs a semantic search and displays the matching original text.

---

##  Performance Benchmarks

Tested on a database of **10,000 high-dimensional vectors** (384-d):

*   **Core Search Time (C Engine, using 5 threads):** ~12.19 ms 
*   **Total Latency (Python + Network + C):** ~23.63 ms
*   **Throughput:** ~500,000 vector comparisons per second.

> [!NOTE]
> The tiny 3.3ms overhead between the C engine and the Python client demonstrates the efficiency of our custom TCP protocol and the low overhead of the `sentence-transformers` inference.

--- 

##  Project Structure
```text
mini_database/
├── include/           # Header files (.h)
├── src/               # C Database Engine
│   ├── db_server.c    # Server entry point
│   ├── storage.c      # Binary I/O & fsync management
│   └── database.c     # K-NN logic & distance calculations
├── scripts/           # Artificial Intelligence layer
│   ├── main_ai.py     # Python -> C Bridge
│   ├── metadata.json  # ID/Text mapping
│   └── requirements.txt
├── Makefile           # Build system
└── database.db        # Persistent binary data