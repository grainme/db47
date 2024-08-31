# db47 - ACID-Compliant Key-Value Store

## Project Overview

This project is an initial implementation of a simple key-value store written in C, C++. The focus is on ensuring basic ACID (Atomicity, Consistency, Isolation, Durability) properties to manage data integrity during read and write operations.

## Features

- **Atomicity**: Ensures that write operations are atomic by using temporary files and renaming them once the operation is complete.
- **Consistency**: Basic validation is in place to ensure that data remains consistent during operations.
- **Isolation**: File locking (`flock`) is used to prevent simultaneous writes, ensuring isolation between operations.
- **Durability**: The use of `fsync` guarantees that data is written to disk, ensuring durability even in the event of a system crash.

## Current State

- The project currently supports basic `get` and `set` operations for managing key-value pairs.
- The ACID properties have been partially implemented, with a focus on Atomicity and Durability.
- Concurrency control is handled through file locking, but improvements are needed to handle more complex scenarios.
- Error handling is basic, with plans to enhance robustness and cover more edge cases.

## How to Use

### Build the Project

To compile the project, use the following command:

```sh
gcc -o kv_store main.c
```
## Run the Project
### You can use the following commands to set and get key-value pairs:

```sh
./kv_store set key value
./kv_store get key

# Set a value
./kv_store set name "John Doe"

# Get a value
./kv_store get name
```

## Next Steps
- [ ] Enhance Consistency and Isolation: Improve the handling of concurrent operations and add more validation checks.
- [ ] Refactor Code: Modularize the codebase to improve readability and maintainability.
- [ ] Improve Error Handling: Ensure that all possible errors are handled gracefully with detailed messages.
- [ ] Add Unit Tests: Implement unit tests to verify the correctness of individual components.
