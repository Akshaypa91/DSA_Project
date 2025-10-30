# MiniGit - Version Control System in C

A simplified Git implementation built in C for learning Data Structures & Algorithms concepts.

---

## Features

- 📦 Initialize repository
- ➕ Stage files for commit
- 💾 Create commits with messages
- 📜 View commit history
- 🔄 Checkout commits/branches
- 🌿 Create and manage branches
- 🔀 Merge branches
- 📊 Compare commits (diff)
- 🌐 Push/pull to remote

---

## Quick Start

```bash
# Clone and build
git clone https://github.com/Akshaypa91/DSA_Project.git
cd DSA_Project
make

# Initialize repository
./mini_git init

# Create and commit a file
echo "Hello World" > file.txt
./mini_git add file.txt
./mini_git commit "Initial commit"

# View history
./mini_git log
```

---

### Build

```bash
# Compile
make

# Clean build artifacts
make clean
```

---

## Usage

## Commands

| Command | Usage | Description |
|---------|-------|-------------|
| `init` | `./mini_git init` | Initialize repository |
| `add` | `./mini_git add <file>` | Stage file |
| `commit` | `./mini_git commit "msg"` | Create commit |
| `log` | `./mini_git log` | View history |
| `status` | `./mini_git status` | Show staging area |
| `branch` | `./mini_git branch <name>` | Create branch |
| `branches` | `./mini_git branches` | List branches |
| `checkout` | `./mini_git checkout <branch/commit>` | Switch branch/commit |
| `merge` | `./mini_git merge <branch>` | Merge branch |
| `diff` | `./mini_git diff <id1> <id2>` | Compare commits |
| `push` | `./mini_git push` | Push to remote |
| `pull` | `./mini_git pull` | Pull from remote |

---

## Examples

### Basic Workflow

```bash
# Initialize
./mini_git init

# Create and stage file
echo "Code here" > main.c
./mini_git add main.c

# Commit
./mini_git commit "Add main.c"

# View history
./mini_git log
```

### Branch Workflow

```bash
# Create feature branch
./mini_git branch feature
./mini_git checkout feature

# Make changes
echo "New feature" > feature.c
./mini_git add feature.c
./mini_git commit "Add feature"

# Merge to main
./mini_git checkout main
./mini_git merge feature
```

---

## Project Structure

```
DSA_Project/
├── main.c          # Entry point
├── init.c          # Repository init
├── add.c           # Staging
├── commit.c        # Commits
├── log.c           # History
├── checkout.c      # Checkout
├── branch.c        # Branches
├── merge.c         # Merging
├── diff.c          # Comparison
├── remote.c        # Remote ops
├── utils.c         # Utilities
├── utils.h         # Headers
└── Makefile        # Build config
```

### Repository Structure

```
.minigit/
├── commits/        # Commit objects
├── refs/           # Branch refs
├── staging/        # Staged files
└── HEAD            # Current pointer
```

---

## How It Works

**Data Structures:**
- Linked Lists → Commit chains
- Hash Tables → File tracking
- Trees → Directory structure

**Storage:**
- Each commit stores file snapshots
- Branches point to commits
- HEAD tracks current location

---

## Contributing

1. Fork the repo
2. Create feature branch: `git checkout -b feature-name`
3. Commit changes: `git commit -m 'Add feature'`
4. Push: `git push origin feature-name`
5. Open Pull Request

---

## Author

****

DSA Project - Second Year (2025-2026)

---

## Acknowledgments

- Inspired by Git
- Built for educational purposes
- Part of DSA coursework