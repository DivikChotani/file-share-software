
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <string>
#include <regex>
#include <fstream>
#include <map>
#include <algorithm>

namespace fs = std::filesystem;
using namespace std;

struct trip {
    std::string name;  // Level or depth of the structure
    int level;
    enum Type {
        FILE,
        DIRECTORY
    } type;
};

vector<trip> fileTree;
vector<trip> fileTreePretty;

// Load ignore patterns from a custom ignore file
std::vector<std::regex> loadIgnorePatterns(const std::string& ignoreFile) {
    std::vector<std::regex> patterns;
    std::ifstream file(ignoreFile);

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line[0] != '#') {
                patterns.emplace_back(line);
            }
        }
        file.close();
    } else {
        std::cerr << "Could not open ignore file: " << ignoreFile << std::endl;
    }
    return patterns;
}

// Function to check if a file or directory is ignored by Git
bool isGitIgnored(const fs::path& path, const vector<std::regex>& ignore) {
    std::string checkPath = path.filename().string();

    for (const auto& pattern : ignore) {
        if (std::regex_match(checkPath, pattern)) {
            return true;
        }
    }
    return false;
}

// Recursive function to print the file system, excluding ignored files and `.git`
void printFileSystem(const fs::path& toCommit, const vector<std::regex>& ignorePatterns, int level) {
    //std::cout << "Scanning directory: " << toCommit << " at level: " << level << std::endl;

    for (const auto& entry : fs::directory_iterator(toCommit)) {
        if (isGitIgnored(entry.path(), ignorePatterns)) {
            //std::cout << "Ignored: " << entry.path() << std::endl;
            continue;
        }

        for (int i = 0; i < level; ++i) {
            std::cout << "  ";
        }
        //std::cout << "|-- " << entry.path().filename() << std::endl;

        trip t;
        t.name = entry.path();
        t.level = level;
        trip p;
        p.name = entry.path().filename();
        p.level = level;

        if (entry.is_directory()) {
            t.type = trip::DIRECTORY;
            p.type = trip::DIRECTORY;
        } else {
            t.type = trip::FILE;
            p.type = trip::FILE;
        }

        fileTree.push_back(t);
        fileTreePretty.push_back(p);

        if (entry.is_directory()) {
            printFileSystem(entry.path(), ignorePatterns, level + 1);
        }
    }
}

void printTrip(const trip& t) {
    std::cout << "Name: " << t.name << ", Level: " << t.level 
              << ", Type: " << (t.type == trip::FILE ? "File" : "Directory") << std::endl;
}

// Function to print the contents of a trip vector
void printFileTree(const std::vector<trip>& tree, const std::string& treeName) {
    std::cout << "Contents of " << treeName << ":" << std::endl;
    for (const auto& t : tree) {
        printTrip(t);
    }
}

std::string runCommand(const std::string& command) {
    std::ostringstream output;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to execute command: " << command << std::endl;
        return "";
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output << buffer;
    }
    pclose(pipe);
    return output.str();
}

// Commit files at a specific level and create blobs
std::string readFileContent(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

// Modified commitFiles function to use actual file content
std::map<std::string, std::string> commitFiles(const std::vector<trip>& entries, int level) {
    std::map<std::string, std::string> blobs;
    for (const auto& entry : entries) {
        if (entry.level == level && entry.type == trip::FILE) {
            // Read actual file content
            std::string content = readFileContent(entry.name);
            
            // Write content to git object store
            std::string tempFile = "/tmp/git_blob_" + fs::path(entry.name).filename().string();
            std::ofstream temp(tempFile);
            temp << content;
            temp.close();
            
            // Create blob using git hash-object
            std::string command = "git hash-object -w " + tempFile;
            std::string blobHash = runCommand(command);
            if (!blobHash.empty()) {
                blobHash = blobHash.substr(0, blobHash.find_last_not_of(" \n\r\t") + 1);
                blobs[fs::path(entry.name).filename().string()] = blobHash;
                std::cout << "Created blob " << blobHash << " for " << entry.name << std::endl;
            }
            
            // Cleanup temp file
            std::remove(tempFile.c_str());
        }
    }
    return blobs;
}

// Helper function to create a tree entry string
std::string createTreeEntry(const std::string& mode, const std::string& type, 
                          const std::string& hash, const std::string& name) {
    return mode + " " + type + " " + hash + "\t" + name + "\n";
}

// Modified createTrees function
std::map<std::string, std::string> createTrees(const std::vector<trip>& entries, int level,
                                              const std::map<std::string, std::string>& blobs,
                                              const std::map<std::string, std::string>& childTrees) {
    std::map<std::string, std::string> trees;
    std::map<std::string, std::string> treeContents;
    
    // First, group all entries by their parent directory
    for (const auto& entry : entries) {
        if (entry.level == level && entry.type == trip::DIRECTORY) {
            std::string dirName = entry.name;
            std::ostringstream treeContent;
            
            // Add blobs (files) to tree
            for (const auto& [fileName, hash] : blobs) {
                // Check if the file belongs to this directory
                for (const auto& fileEntry : entries) {
                    if (fileEntry.type == trip::FILE && 
                        fileEntry.level == level + 1 &&
                        fs::path(fileEntry.name).parent_path() == dirName) {
                        if (fs::path(fileEntry.name).filename() == fileName) {
                            treeContent << createTreeEntry("100644", "blob", hash, fileName);
                        }
                    }
                }
            }
            
            // Add child trees (subdirectories)
            for (const auto& [childName, hash] : childTrees) {
                // Check if the directory is a direct child of current directory
                for (const auto& dirEntry : entries) {
                    if (dirEntry.type == trip::DIRECTORY && 
                        dirEntry.level == level + 1 &&
                        fs::path(dirEntry.name).parent_path() == dirName) {
                        if (fs::path(dirEntry.name).filename() == childName) {
                            treeContent << createTreeEntry("040000", "tree", hash, childName);
                        }
                    }
                }
            }
            
            // Store tree content for this directory
            if (!treeContent.str().empty()) {
                treeContents[fs::path(dirName).filename().string()] = treeContent.str();
            }
        }
    }
    
    // Create trees using git mktree
    for (const auto& [dirName, content] : treeContents) {
        // Write tree content to temporary file
        std::string tempFile = "/tmp/git_tree_" + dirName;
        std::ofstream temp(tempFile);
        temp << content;
        temp.close();
        
        // Create tree using git mktree
        std::string command = "git mktree < " + tempFile;
        std::string treeHash = runCommand(command);
        if (!treeHash.empty()) {
            treeHash = treeHash.substr(0, treeHash.find_last_not_of(" \n\r\t") + 1);
            trees[dirName] = treeHash;
            std::cout << "Created tree " << treeHash << " for " << dirName << std::endl;
        }
        
        // Cleanup temp file
        std::remove(tempFile.c_str());
    }
    
    return trees;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <directory>" << std::endl;
        return 1;
    }

    fs::path toCommit = fs::absolute(argv[1]);
    std::cout << "Path to commit: " << toCommit << std::endl;
    fs::path baseRoot = toCommit.filename();
    if (!fs::exists(toCommit)) {
        std::cerr << "Error: Path does not exist: " << toCommit << std::endl;
        return 1;
    }
    string branch = argv[2];
    trip t;
    t.name = toCommit;
    t.level = 0;
    t.type = trip::DIRECTORY;
    fileTree.push_back(t);
    trip p;
    p.name = baseRoot;
    p.level = 0;
    p.type = trip::DIRECTORY;
    fileTreePretty.push_back(p);

    auto ignorePatterns = loadIgnorePatterns("../.customignore");
    printFileSystem(toCommit, ignorePatterns, 1);

    printFileTree(fileTree, "fileTree");
    printFileTree(fileTreePretty, "fileTreePretty");

    int maxLevel = 0;
    for (const auto& entry : fileTreePretty) {
        maxLevel = std::max(maxLevel, entry.level);
    }

    std::map<std::string, std::string> blobs;
    std::map<std::string, std::string> trees;
    
    // Process from bottom-up
    for (int level = maxLevel; level >= 0; --level) {
        std::cout << "Processing level " << level << std::endl;
        
        // Get blobs for current level
        auto levelBlobs = commitFiles(fileTree, level);
        blobs.insert(levelBlobs.begin(), levelBlobs.end());
        
        // Create trees for current level using all available blobs and child trees
        auto levelTrees = createTrees(fileTree, level, blobs, trees);
        trees.insert(levelTrees.begin(), levelTrees.end());
    }

    // The root tree should be in trees map with the key "root"
    
    std::string rootKey = baseRoot.string();

    if (trees.find(rootKey) != trees.end()) {
        std::cout << "Root tree hash: " << trees[rootKey] << std::endl;
        
        // Create a new tree entry containing the root tree
        std::string treeEntry = createTreeEntry("040000", "tree", trees[rootKey], rootKey);
        
        // Write the tree entry to a temporary file
        std::string tempFile = "/tmp/final_tree";
        std::ofstream temp(tempFile);
        temp << treeEntry;
        temp.close();
        
        // Create the final tree using mktree
        std::string command = "git mktree < " + tempFile;
        std::string finalTreeHash = runCommand(command);
        finalTreeHash = finalTreeHash.substr(0, finalTreeHash.find_last_not_of(" \n\r\t") + 1);
        std::cout << "Final tree hash: " << finalTreeHash << std::endl;
        
        // Create commit using the final tree hash
        std::ostringstream commitCommand;
        commitCommand << "git commit-tree " << finalTreeHash 
                    << " -m \"Initial commit of directory structure\"";
        
        std::string commitHash = runCommand(commitCommand.str());
        if (!commitHash.empty()) {
            commitHash = commitHash.substr(0, commitHash.find_last_not_of(" \n\r\t") + 1);
            std::cout << "Created commit: " << commitHash << std::endl;

            std::string repo = "DivikChotani/file-share";

            // Properly quote the token
            std::string token = "";
            std::string push = "git push --force \"https://" + token + "@github.com/" + repo + ".git\" " +
                            commitHash + ":refs/heads/" + branch;
            std::cout << "Executing: " << push << std::endl;
            runCommand(push);
        }

        // Cleanup temp file
        std::remove(tempFile.c_str());
    } 
    else {
        std::cerr << "Error: Root tree not found" << std::endl;
    }

    return 0;
    
}