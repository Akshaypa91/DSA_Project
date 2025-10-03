# MiniGit (DSA Version Control System)

MiniGit is a simplified version control system implemented in C. It allows you to track files, commit changes, manage branches, checkout commits, and view commit history — similar to Git, but simplified for learning and experimentation.

---

## Prerequisites

- GCC compiler (`gcc`) installed
- Linux or Windows environment
- Terminal / Command line access

---

## Project Structure

DSA_Project_SY/
├── main.c
├── init.c
├── add.c
├── commit.c
├── log.c
├── checkout.c
├── branch.c
├── merge.c
├── diff.c
├── remote.c
├── utils.c
├── utils.h
├── Makefile
└── README.md


---

## Quick Start

Run these commands to quickly try the project:

```bash
# Initialize repository
./mini_git init

# Create a file
echo "Hello world" > file1.txt

# Add file to staging area
./mini_git add file1.txt

# Commit changes with a message
./mini_git commit "Initial commit"

# View commit history
./mini_git log


Build Instructions

Open a terminal in the project directory.

Clean previous builds (optional):

make clean


Compile the project using the Makefile:

make


This will generate the executable:

mini_git

Running MiniGit
Interactive Menu Mode

Run the program and follow the menu:

./mini_git


You will see options:

1. Init Repository
2. Add File
3. Show Staging Area
4. Commit
5. Log Commits
6. Checkout Commit
7. Create Branch
8. List Branches
9. Checkout Branch
10. Merge Branch
11. Diff Between Commits
12. Push to Remote
13. Pull from Remote
0. Exit

Command-Line Mode

You can also execute commands directly:

# Initialize repository
./mini_git init

# Add a file to staging area
./mini_git add file1.txt

# Commit changes with a message
./mini_git commit "Initial commit"

# View commit history
./mini_git log

Example Usage
# Initialize repository
./mini_git init
# Output: Initialized empty MiniGit repository

# Create a file
echo "Hello world" > file1.txt

# Add file to staging area
./mini_git add file1.txt
# Output: Added 'file1.txt' to staging area

# Commit the changes
./mini_git commit "Initial commit"
# Output: Committed as c131 : Initial commit

# View commit history
./mini_git log
# Output:
# Commit ID: c131
# Message: Initial commit
# Timestamp: Thu Oct 3 17:30:00 2025
# -----------------------------

Features

Initialize a repository (.minigit folder)

Add files to staging area

Commit changes with messages and timestamps

View commit history (log)

Simple snapshot system for file versioning

Branch creation, checkout, and merging

Diff between commits

Push and pull to remote folder

Cleaning Up

Remove compiled files and executable:

make clean