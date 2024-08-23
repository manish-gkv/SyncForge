
# SyncForge

SyncForge is a simple version control system built using C++. It mimics the basic functionalities of Git, such as initializing a repository, adding files to the staging area, committing changes, viewing commit logs, and showing differences between commits.

## Features

- **Initialization**: Create a new SyncForge repository.
- **Add**: Stage files for commit.
- **Commit**: Record the changes in the repository.
- **Log**: View the commit history.
- **Diff**: Show the differences between the current and previous commits.

## Installation

### Prerequisites
- C++ compiler (e.g., `g++`)
- MSYS2 (for Windows users)
- SHA1 library (You have to include the `sha1.h` header and its implementation in this project.)

### Build
Compile the project using the following command:
```bash
g++ SyncForge.cpp -o SyncForge
```

## Usage

### 1. Initialize a Repository

To initialize a new SyncForge repository, use the `init` command:
```bash
./SyncForge init
```
This creates a `.SyncForge` directory in your current working directory.

### 2. Add Files to the Staging Area

To stage a file for commit, use the `add` command:
```bash
./SyncForge add <filename>
```

### 3. Commit Changes

To commit the staged changes, use the `commit` command:
```bash
./SyncForge commit -m "Your commit message"
```

### 4. View Commit History

To view the commit history, use the `log` command:
```bash
./SyncForge log
```

### 5. Show Differences Between Commits

To see the differences between a commit and its parent, use the `diff` command followed by the commit hash:
```bash
./SyncForge diff <commit_hash>
```

## How It Works

- **Initialization**: Creates the necessary directories and files (`.SyncForge`, `objects`, `Head`, `index`) for the repository.
  
- **Adding Files**: Computes the SHA1 hash of the file content, stores it in the `objects` directory, and updates the `index` file with the file path and its corresponding hash.
  
- **Committing**: Creates a commit object that includes the current time, commit message, files from the staging area (with their hashes), and the parent commit hash. The commit object is serialized and stored in the `objects` directory. The `Head` file is updated to point to the new commit.

- **Viewing Logs**: Displays a list of commits from the latest to the earliest, including commit hash, date, and message.

- **Showing Diffs**: Compares the differences between the current commit and its parent, highlighting added and removed lines.

## Contributing

Contributions are welcome! Please fork this repository and submit pull requests.

## License

This project is licensed under the MIT License.

## Author

- **Manish** - *Initial work* - [GitHub Profile](https://github.com/manish-gkv)
