#include <ark.h>
#include <string>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <iostream>
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>

std::string normalizePath(const std::string& p) {
    return std::filesystem::path(p).lexically_normal().generic_string();
}

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return ""; // all whitespace

    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string arkDir(){
    std::filesystem::path current = std::filesystem::current_path();

    while (true) {
        if (std::filesystem::exists(current / ".ark") && std::filesystem::is_directory(current / ".ark")) {
            return current; // found repo root
        }

        if (current.has_parent_path() && current.string() != "/") {
            current = current.parent_path();
        } else {
            throw std::runtime_error("Not inside an Ark repository.");
        }
    }
    return current;
}

std::unordered_set<std::string> loadIgnoreFiles(){
  std::string repo_root = arkDir();
  std::vector<std::string> paths;
  paths.push_back(repo_root);
  std::unordered_set<std::string> ignored_patterns;
  for(int i = 0;i < paths.size();i++){
    std::string path = paths[i];
    std::unordered_set<std::string> temp = loadIgnoreFile(path);
    ignored_patterns.insert(temp.begin(),temp.end());
    for(const auto& entry:std::filesystem::directory_iterator(path)){
      if( std::filesystem::is_directory(entry)){
          paths.push_back(entry.path().string());
      }
    }
  }
    return ignored_patterns;
}

std::unordered_set<std::string> loadIgnoreFile(const std::string& path) {
    std::unordered_set<std::string> patterns;

    std::string file = path + "/.arkignore";
    if (!(std::filesystem::exists(file) && std::filesystem::is_regular_file(file)))
        return patterns;


    std::ifstream in(file);
    if (!in) {
        std::cerr << "error reading file " << file << "\n";
        return patterns;
    }

    std::string line;
    while (getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        patterns.insert(normalizePath(path+"/"+line));
    }

    return patterns;
}

bool isIgnored(const std::string& path,const std::unordered_set<std::string>& ignored_patterns){
   for (const auto& pattern : ignored_patterns) {
        if (path == pattern) {
            return true;
        }

        if (!pattern.empty() && pattern.back() == '/') {
            if (path.rfind(pattern, 0) == 0) { 
                return true;
            }
        }
  }
  return false;
}


std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);

    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string getEditor() {
    const char* editor = std::getenv("GIT_EDITOR");
    if (!editor) editor = std::getenv("VISUAL");
    if (!editor) editor = std::getenv("EDITOR");
    if (!editor) editor = "vi";
    return std::string(editor);
}

void openEditor(const std::string& editor, const std::string& file) {
    std::string cmd = editor + " " + file;
    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "Editor exited with code " << ret << std::endl;
    }
}

std::string createTempFile() {
    std::string temp_file = std::filesystem::temp_directory_path() / "ark_commit_msg.txt";
    std::ofstream temp(temp_file); // create empty file
    return temp_file;
}

std::string readFileContent(const std::string& file) {
    std::ifstream in(file);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}


std::string getCommitMessageFromEditor() {
    std::string editor = getEditor();
    std::string tempFile = createTempFile();
    openEditor(editor, tempFile);
    std::string message = readFileContent(tempFile);
    std::filesystem::remove(tempFile); // cleanup
    return message;
}



std::string getMode(const std::filesystem::path& path) {
    struct stat st;
    if (lstat(path.c_str(), &st) != 0) {
        return "0"; // error fallback
    }

    if (S_ISREG(st.st_mode)) {
        if (st.st_mode & S_IXUSR) return "100755"; // executable file
        else return "100644"; // regular file
    } else if (S_ISDIR(st.st_mode)) {
        return "040000"; // directory
    } else if (S_ISLNK(st.st_mode)) {
        return "120000"; // symlink
    }
    return "0"; // unknown type
}

bool isIndexSameAsCommit(const std::string& commit_hash){
  std::unordered_map<std::string, std::pair<std::string, std::string>> index = loadIndex();

    Commit* commit = new Commit();
    commit->loadFromDisk(commit_hash);
    std::unordered_map<std::string, std::pair<std::string,std::string>> latest_committed_files = commit->tree->flatten();

    for (const auto& [name, obj] : index) {
        const std::string& index_hash = obj.first;
        auto it = latest_committed_files.find(name);

        if (it == latest_committed_files.end() || it->second.first != index_hash) {
          return false;
        }
    }
    return true;

}

std::unordered_map<std::string,std::string> loadConfig(){
  //TODO: finish this please
  std::string repo_root = arkDir();
  std::unordered_map<std::string,std::string> config;
  std::string local_config_file = repo_root + "/.ark/config";
  std::cout<<local_config_file<<"\n";
  return config;
}


