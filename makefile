CC = g++
CFLAGS = -Wall -Wextra -O2 -std=c++17

SRC_A = main.cpp
SRC_R = receive.cpp
BIN_A = share-file
BIN_R = receive
GIT_REPO = git-repo

all: $(BIN_A) $(BIN_R) init-git-repo

$(BIN_A): $(SRC_A)
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_R): $(SRC_R)
	$(CC) $(CFLAGS) -o $@ $<

init-git-repo:
	@echo "Initializing Git repository in $(GIT_REPO)..."
	@mkdir -p $(GIT_REPO)
	@if [ ! -d "$(GIT_REPO)/.git" ]; then \
		cd $(GIT_REPO) && \
		git init && \
		git remote add origin https://github.com/DivikChotani/file-share.git && \
		git branch -M main && \
		touch README.md && \
		git add README.md && \
		git commit -m "Initial commit" && \
		echo "Git repository initialized."; \
	else \
		echo "Git repository already exists."; \
	fi

clean:
	@echo "Cleaning up..."
	@rm -f $(BIN_A) $(BIN_R)
	@rm -rf $(GIT_REPO)
	@echo "Clean complete."

