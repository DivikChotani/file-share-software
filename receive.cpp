
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <string>
#include <regex>
#include <fstream>
#include <map>
#include <algorithm>
#include <unistd.h>

namespace fs = std::filesystem;
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
void moveFiles(std::string moveFile, std::string destination){
    std::ifstream file(moveFile);

    if(file.is_open()){
        std::string line;
        while(std::getline(file, line)){
            runCommand("rm -rf"+ line);
        }
    }

}
int main(int argc, char* argv[]) {
    std::string t = argv[1];

    for (int i = 0; i < argc; i++){
        std::cout << argv[i] << std::endl;
    }
    // Clean up any existing state
    runCommand("git fetch --prune");
    runCommand("ls | sort > .pre-pull.txt");
    
    // Create new branch
    std::string branch = "git checkout -b " + t;
    runCommand(branch);
    runCommand("git commit -m \"Remove all files from branch\"");    
    // Push empty branch to establish tracking
    std::string repo = "DivikChotani/file-share";
    std::string token = "";
    std::string push = "git push --force \"https://" + token + "@github.com/" + repo + ".git\" " + t;
    runCommand(push);
    
    sleep(2);
    
    // Get hash AFTER our initial push
    std::string initialStatus = runCommand("git ls-remote origin refs/heads/" + t);
    std::string initialHash;
    if (!initialStatus.empty()) {
        initialHash = initialStatus.substr(0, 40);
    } else {
        std::cerr << "Failed to get initial hash" << std::endl;
        return 1;
    }
    
    std::cout << "Waiting for changes after initial hash: " << initialHash << std::endl;
    
    // Wait for changes with timeout
    int attempts = 0;
    const int MAX_ATTEMPTS = 60;
    std::string newHash = initialHash;
    
    while(newHash == initialHash && attempts < MAX_ATTEMPTS) {
        sleep(5);
        std::string status = runCommand("git ls-remote origin refs/heads/" + t);
        if (!status.empty()) {
            newHash = status.substr(0, 40);
            std::cout << "Current hash: " << newHash << std::endl;
        }
        attempts++;
        std::cout << "Waiting for changes... attempt " << attempts << std::endl;
    }
    
    if (attempts >= MAX_ATTEMPTS) {
        std::cerr << "Timeout waiting for changes" << std::endl;
        return 1;
    }
    
    if (newHash != initialHash) {
        std::cout << "Changes detected! New hash: " << newHash << std::endl;
        
        // Get the tree contents to find the root directory name
        runCommand("git fetch --force origin " + t);
        std::string treeContents = runCommand("git ls-tree --name-only " + newHash);
        
        // Clean up the directory name - remove any whitespace
        std::string rootDirName;
        std::istringstream iss(treeContents);
        std::getline(iss, rootDirName);
        rootDirName.erase(0, rootDirName.find_first_not_of(" \t\n\r"));
        rootDirName.erase(rootDirName.find_last_not_of(" \t\n\r") + 1);
        
        std::cout << "Found root directory name: '" << rootDirName << "'" << std::endl;
        
        // Reset to the new commit
        runCommand("git reset --hard FETCH_HEAD");
        
        //Process new files if destination provided
        if(argc == 3) {
            // Instead of using ls/comm/diff, directly check if directory exists and move it
            std::string destination = argv[2];
            std::string sourcePath = rootDirName;
            std::string destPath = std::string(argv[2]) + "/" + rootDirName;
            
            if (fs::exists(sourcePath)) {
                std::cout << "Moving '" << sourcePath << "' to '" << destPath << "'" << std::endl;
                // Remove destination if it exists
                if (fs::exists(destPath)) {
                    std::cout << "Removing existing destination directory" << std::endl;
                    fs::remove_all(destPath);
                }
                // Move directory to destination
                try {
                    fs::rename(sourcePath, destPath);
                    std::cout << "Successfully moved directory" << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error moving directory: " << e.what() << std::endl;
                }
            } else {
                std::cerr << "Source directory '" << sourcePath << "' not found" << std::endl;
            }
        }
    }
    
    // Cleanup
    runCommand("ls | sort > .post.txt");
    runCommand("comm -3 .pre-pull.txt .post.txt > diff.txt");
    runCommand("rm .pre-pull.txt .post.txt diff.txt");
    runCommand("git checkout main");
    runCommand("git branch -D " + t);
    runCommand("git push origin --delete " + t);
    
    return 0;
}