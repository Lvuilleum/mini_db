# Mini Database in C

This project is a **simple database engine written in C**.
The goal is to learn low-level programming concepts such as:

* memory management
* file storage
* parsing user input
* modular C project architecture

The project is currently **in early development**.

---

# Project Goal

The final program will allow users to interact with a small database from the command line.

Example of the expected interface:

```
./mydb data.db

db > insert 1 Alice 20
Executed.

db > insert 2 Bob 30
Executed.

db > select
1 Alice 20
2 Bob 30

db > delete 1
row 1 deleted 

db > .exit
```

The data will be stored in a file (`data.db`) so that it persists between executions.

---

# Project Structure

```
mini-db/
│
├── main.c
├── parser.c
├── parser.h
├── database.c
├── database.h
├── storage.c
├── storage.h
```

Each module has a specific responsibility:

* **main.c** → program entry point and CLI loop
* **parser** → parses user input into commands
* **database** → handles database logic
* **storage** → manages file persistence

---

# Command to run the project
gcc -Wall -Wextra -Werror -Iinclude src/*.c -o mydb

# Planned Features

* command-line interface
* `insert` command
* `select` command
* `delete` command
* persistent file storage
* modular C architecture
* basic indexing (future improvement)

---

# Learning Objectives

This project is built to practice:

* C programming
* struct and enum usage
* pointers
* modular code design
* file I/O
* basic database internals
