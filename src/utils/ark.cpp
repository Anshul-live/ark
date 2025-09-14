#include <ark.h>
#include <string>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <iostream>
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <ctime>
#include <iomanip>

std::string getTimezoneOffset() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm utc_tm = *std::gmtime(&t);
    std::tm local_tm = *std::localtime(&t);

    auto diff = std::mktime(&local_tm) - std::mktime(&utc_tm);

    int hours   = diff / 3600;
    int minutes = (std::abs(diff) % 3600) / 60;

    std::ostringstream oss;
    oss << (diff >= 0 ? '+' : '-')
        << std::setw(2) << std::setfill('0') << std::abs(hours)
        << std::setw(2) << std::setfill('0') << minutes;

    return oss.str();
}

std::string normalizePath(const std::string& p) {
    return std::filesystem::path(p).lexically_normal().generic_string();
}
std::string trim(const std::string& s,std::string del) {
    size_t start = s.find_first_not_of(del);
    if (start == std::string::npos)
        return "";

    size_t end = s.find_last_not_of(del);
    return s.substr(start, end - start + 1);
}

std::string removeCharactersFromString(const std::string& s,std::string del){
  std::unordered_set<char> st(del.begin(),del.end());
  std::string result;
  for(char c:s){
    if(st.count(c)){
      continue;
    }
      result+=c;
  }
  return result;
}

std::string replaceCharacter(const std::string& s,char original,char replacement){
  std::string result;
  for(char c:s){
    if(c == original)
      c = replacement;

    result.push_back(c);
  }
  return result;
}
std::string removeExtraConsecutiveOccurences(const std::string& s,char c,int limit){
  std::string result;
  int count = 0;
  for(int i = 0;i < s.length();i++){
    char el = s[i];
    if(el == c){
      if(count <= limit){
        result.push_back(el);
        count++;
      }
      continue;
    }
    count = 0;
    result.push_back(el);
  }
  return result;
}

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return ""; // all whitespace

    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}
std::string toLower(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return result;
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

void loadConfigFile(const std::string& config_file,std::unordered_map<std::string,std::unordered_map<std::string,std::vector<std::string>>> &config){
 std::ifstream in(config_file);
  if(!in){
    return;
  }
  std::string section_name = "";
  std::string line;
  while(getline(in,line)){
    if(line.empty() || line[0] == '3')
      continue;
    line = toLower(trim(line));

    if(line[0] == '['){
      line = removeCharactersFromString(line,"[]\"");
      line = removeExtraConsecutiveOccurences(line,' ',1);
      line = replaceCharacter(line,' ','.');
      section_name = trim(line);
    }
    else if(line[0] >= 'a' && line[0] <= 'z'){
      std::vector<std::string> content = split(line,'=');
      if(content.size() != 2 ){
        std::cerr<<"invalid format on line\n";
        continue;
      }
      std::string field_name = trim(content[0]);
      std::string value = trim(removeCharactersFromString(content[1],"\""));
      config[section_name][field_name].push_back(value);
    }
    else{
      std::cerr<<"invalid symbols in config file\n";
    }
  }
    in.close();
}

std::unordered_map<std::string,std::unordered_map<std::string,std::vector<std::string>>> loadConfig(){
  //TODO: finish this please
  std::string repo_root = arkDir();
  std::string local_config_file = repo_root + "/.ark/config";
  std::string user_config_file1 = "~/.arkconfig";
  std::string user_config_file2 = "~/config/ark/config";
  std::cout<<"loading local file"<<local_config_file<<"\n";

  std::unordered_map<std::string,std::unordered_map<std::string,std::vector<std::string>>> config;

  loadConfigFile(local_config_file,config);
  loadConfigFile(user_config_file1,config);
  loadConfigFile(user_config_file2,config);

  return config;
}


