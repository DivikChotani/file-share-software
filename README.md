# File Share Utility

This project provides a utility for sharing and receiving directories via a command-line interface. It uses a Git-based approach to facilitate easy file transfers between users. The workflow includes building the project using `make`, running the `./share` program, and following prompts to either share or receive files.

---

## Features
- Share directories with specific users or branches.
- Receive directories to a specified location.
- User-friendly prompts for seamless interaction.
- Uses Git repositories for version control and file management.

---

## Requirements
1. **Git**: Ensure Git is installed on your system.
2. **C++ Compiler**: The project requires `g++` or any C++17-compatible compiler.
3. **GitHub Personal Access Token (PAT)**:
   - A GitHub PAT is required for authentication when accessing the `git-repo`.
   - See [How to Generate a PAT](#personal-access-token-pat-requirement) for more details.

---

## How to Build and Use

### 1. Build the Project
Run the following command in the root of the project directory to compile the binaries:

```bash
make all

# File Sharing Program

## 1. Personal Access Token (PAT) Requirement

Before running the program, ensure you have a valid GitHub Personal Access Token (PAT). This token is required to authenticate with the git-repo.

### Steps to Generate a PAT:

1. Log in to your GitHub account.
2. Navigate to **Settings > Developer Settings > Personal Access Tokens**.
3. Generate a new token with the following scopes:
   - `repo` (for accessing repositories).
4. Copy the generated token and keep it secure.
5. Export the PAT as an environment variable so the program can use it:

```bash
export GITHUB_PAT=your_personal_access_token
```

---

## 2. Navigate to the Project Directory
Ensure you're in the correct directory where the compiled programs (`share-file` and `receive`) and the Git repository (`git-repo`) are located.

---

## 3. Run the `share` Program
Execute the `./share` script to initiate sharing or receiving files.

### Example Workflow

#### Case 1: Receiving Files
Run the following command:

```bash
./share
```

You will see the following prompts:

```plaintext
Enter 'share' or 'receive': receive
Enter the path where you want to receive the directory: ~
Enter a temporary receiver name: sigma
```

- **Prompt 1**: Choose whether to share or receive. Type `receive` to receive files.
- **Prompt 2**: Enter the directory where the received files should be saved (e.g., `~` for your home directory).
- **Prompt 3**: Provide a temporary receiver name (e.g., `sigma`).

The program will then execute the logic for receiving files.

#### Case 2: Sharing Files
Run the same command:

```bash
./share
```

Follow the prompts:

```plaintext
Enter 'share' or 'receive': share
Enter the directory to share: /path/to/share
Enter who you want to share it to: recipient-branch
```

- **Prompt 1**: Choose `share` to share files.
- **Prompt 2**: Provide the path of the directory you want to share.
- **Prompt 3**: Enter the name of the recipient branch or user.

The program will handle sharing the specified directory.

---

## 4. Cleanup (Optional)
To remove the compiled binaries and reset the project, run:

```bash
make clean
```

---

## Project Structure
The directory structure is as follows:

```plaintext
file-share/
│
├── main.cpp            # Code for the 'share-file' program
├── receive.cpp         # Code for the 'receive' program
├── makefile            # Build configuration
├── setup.sh            # Script to install binaries
├── .gitignore          # Excludes unnecessary files from Git
├── git-repo/           # Git repository for sharing/receiving files
└── README.md           # Documentation
```

---

## Notes
1. Ensure the `git-repo` directory is initialized as a valid Git repository.
   - The `makefile` includes a target (`init-git-repo`) to handle initialization.
2. Add `.DS_Store` and `.vscode` to `.gitignore` to exclude unnecessary files from the repository.
3. Customize the `makefile` or `setup.sh` script to adjust the installation directory or other configurations.

---

## Troubleshooting

### Error: `$GIT_REPO_PATH is not a Git repository`
- Ensure the `git-repo` directory exists and is initialized with `git init`.

### `share` or `receive` program not found
- Verify the binaries are correctly built using `make all` and are located in the project directory.

---

Feel free to contribute to this project or raise issues via GitHub!

