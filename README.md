# Mini Database in C

Mini relational-style database written in C, with a simple TCP client/server architecture.

## Overview

This project is built to practice low-level systems concepts:

- memory management
- file I/O and persistence
- socket programming (client/server)
- command parsing
- modular C architecture

The server stores rows in `database.db` and the client provides an interactive `db>` prompt.

## Current Architecture

- `db_server`: accepts TCP connections on port `8080`, parses and executes commands.
- `db_client`: interactive CLI that sends commands to the server and prints server responses.
- persistent storage: row data is saved in `database.db`.
- in-memory id index: a hash-table index is rebuilt from disk on startup for faster lookups.

## Supported Commands

From the client prompt:

- `insert <id> <username> <age>`
- `select` (list all active rows)
- `select <id>` (fetch one row by id)
- `delete <id>`
- `update <id> <username> <age>`
- `help` or `.help`
- `.exit`

Example session:

```text
db> insert 1 Alice 20
row inserted with sucess

db> select
1 Alice 20

db> select 1
1 Alice 20

db> update 1 Alicia 21
updated with success

db> delete 1
deleted with success
```

## Project Structure

```text
mini_database/
├── include/
│   ├── database.h
│   ├── parser.h
│   ├── protocol.h
│   └── storage.h
├── src/
│   ├── database.c
│   ├── db_client.c
│   ├── db_server.c
│   ├── parser.c
│   ├── server.c
│   └── storage.c
├── makefile
├── README.md
└── database.db
```

## Build And Run

Build both binaries:

```bash
make
```

Run server:

```bash
make run-server
```

Run client (in a second terminal):

```bash
make run-client
```

Clean binaries:

```bash
make clean
```

## Notes

- Current protocol is plain text over TCP.
- `select` and `select <id>` results are sent by the server and displayed on the client side.
- The project is educational and not production-hardened yet.
