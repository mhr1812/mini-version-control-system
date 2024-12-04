#include <bits/stdc++.h>
#include <dirent.h>
#include <sys/stat.h>    // imported to create directory
#include <sys/types.h>   // imported to create directory
#include <fstream>       // filehandling
#include <openssl/sha.h> // SHA
#include <ctime>         // using for timestamp
#include <filesystem>
#include "add.h"
#include <zlib.h>
using namespace std;

namespace fs = filesystem;
bool vcs = false; // indicating if there is vcsfolder cerated
int versionNo;
string branchName = "main";
struct commitData{
    int commitNo;
    char commitSHA[50], parentcommitSHA[50], message[200], timeStamp[50], hexCode[10];
};
map<int, commitData> commitInfoMap; // <commitNo, struct commitData>
map<string, int> commitHexMap;      // <hex, commitNo>

void loadCommitData(){
    FILE *commitFile;
    commitFile = fopen("./.mygit/commit.txt", "r");
    commitData cmt;
    while(fread(&cmt, sizeof(commitData), 1, commitFile)){
        commitInfoMap[cmt.commitNo] = cmt;
        commitHexMap[cmt.hexCode] = cmt.commitNo;
    }
    fclose(commitFile);
}

void updateCommitInfoFile(){
    FILE *commitFile = fopen("./.mygit/commit.txt", "w");
    if(!commitFile){
        cout << "Error: Unable to open commit.txt for writing." << endl;
        return;
    }
    for(const auto &entry : commitInfoMap){
        fwrite(&entry.second, sizeof(commitData), 1, commitFile);
    }
    fclose(commitFile);
}

bool checkVcs(){
    string path = "./.mygit";
    char resPath[300];
    realpath(path.c_str(), resPath); // absolute path
    string temp(resPath);
    path = temp;
    struct stat sfile;
    int exist = stat(path.c_str(), &sfile);
    if (exist == -1)
        return vcs = false;
    string fileName = "./.mygit/version.txt";
    fstream file(fileName.c_str());
    file >> temp;
    cout << endl;
    cout << "Current Version no : " << temp << " " << endl;
    versionNo = stoi(temp);
    loadCommitData();
    return vcs = true;
}

void handleInit(){
    if(checkVcs()){
        cout << "Mini Version Control System initialised already...\n";
        return;
    }
    int err = mkdir(".mygit", 0777);
    if(err != -1){
        string temp = "./.mygit/" + to_string(versionNo);
        if(mkdir(temp.c_str(), 0777) == -1){
            cerr << "Error creating vcs -> 0 :  " << strerror(errno) << endl;
            return;
        }
    }
    else{
        cerr << "Error creating vcs :  " << strerror(errno) << endl;
        return;
    }
    cout << "Mini Version Control System initialised...\n";
    vcs = true;

     // Create or initialize version.txt
    string fileName = "./.mygit/version.txt";
    int fd = open(fileName.c_str(), O_WRONLY | O_CREAT, 0666); // Create or open with write permissions
    if(fd != -1){
        string temp = "0";
        write(fd, temp.c_str(), temp.size()); // Write "0" as the initial version number
        close(fd);
    } 
    else{
        cerr << "Error creating version.txt file: " << strerror(errno) << endl;
    }

    // Create or initialize commit.txt
    fileName = "./.mygit/commit.txt";
    fd = open(fileName.c_str(), O_WRONLY | O_CREAT, 0666); // Create or open
    if(fd != -1){
        close(fd); // Nothing to write for now, just close it
    } 
    else{
        cerr << "Error creating commit.txt file: " << strerror(errno) << endl;
    }

    // Create or initialize tracked_history.txt
    fileName = "./.mygit/tracked_history.txt";
    fd = open(fileName.c_str(), O_WRONLY | O_CREAT, 0666); // Create or open
    if(fd != -1){
        close(fd);
    } 
    else{
        cerr << "Error creating tracked_history.txt file: " << strerror(errno) << endl;
    }

    // Create or initialize tracked_current.txt
    fileName = "./.mygit/tracked_current.txt";
    fd = open(fileName.c_str(), O_WRONLY | O_CREAT, 0666); // Create or open
    if (fd != -1){
        close(fd);
    } 
    else{
        cerr << "Error creating tracked_current.txt file: " << strerror(errno) << endl;
    }
}

string generateHex(){
    char chars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    string hex;
    srand((unsigned)time(NULL));
    for (int i = 0; i < 7; i++)
        hex += chars[rand() % 16];
    return hex;
}

void createForPatch(string path1, string path2){
    string command = "diff ";
    string filename, destPath;
    int found = path1.find_last_of(".");
    filename = path1.substr(0, found);
    command = command + path2 + " " + path1 + " >> " + path1 + ".patch";
    system(command.c_str());
    string remove_command = "rm " + path1;
    system(remove_command.c_str());
}

string calculateFileSHA(string cmtData){
    unsigned char completeFileSHA[SHA_DIGEST_LENGTH];
    SHA_CTX shaContext;
    SHA1_Init(&shaContext);
    SHA1_Update(&shaContext, cmtData.c_str(), cmtData.size());
    SHA1_Final(completeFileSHA, &shaContext);
    stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
        ss << hex << setw(2) << setfill('0') << (int)completeFileSHA[i];
    string commitSHA = ss.str();
    // cout << commitSHA << endl;
    return commitSHA;
}

string compressFileContent(const string& filePath){
    int fd = open(filePath.c_str(), O_RDONLY);
    if (fd < 0) {
        cerr << "Error: Could not open file for compression." << endl;
        return "";
    }
    // Get the size of the file
    struct stat fileStat;
    if (fstat(fd, &fileStat) < 0) {
        cerr << "Error: Could not get file stats." << endl;
        close(fd);
        return "";
    }
    size_t fileSize = fileStat.st_size;
    char* fileContent = new char[fileSize];
    // Read the file content into memory
    if (read(fd, fileContent, fileSize) != fileSize) {
        cerr << "Error: Could not read full file content." << endl;
        close(fd);
        delete[] fileContent;
        return "";
    }
    close(fd);
    // Allocate memory for compressed data
    uLong compressedSize = compressBound(fileSize);
    char* compressedData = new char[compressedSize];
    // Compress the file content
    if (compress((Bytef*)compressedData, &compressedSize, (const Bytef*)fileContent, fileSize) != Z_OK) {
        cerr << "Error: Failed to compress file content." << endl;
        delete[] fileContent;
        delete[] compressedData;
        return "";
    }
    string compressedContent(compressedData, compressedSize);
    delete[] fileContent;
    delete[] compressedData;
    return compressedContent;
}
string decompressFileContent(const string& blobPath){
    int fd = open(blobPath.c_str(), O_RDONLY);
    if (fd < 0){
        throw runtime_error("Could not open blob file for reading.");
    }
    vector<char> buffer;
    const size_t bufferSize = 4096; 
    char tempBuffer[bufferSize];
    ssize_t bytesRead;
    while ((bytesRead = read(fd, tempBuffer, bufferSize)) > 0) {
        buffer.insert(buffer.end(), tempBuffer, tempBuffer + bytesRead);
    }
    close(fd);
    string decompressedContent;
    unsigned long decompressedSize = buffer.size() * 4; 
    decompressedContent.resize(decompressedSize);
    int result = uncompress(reinterpret_cast<Bytef*>(&decompressedContent[0]), &decompressedSize, 
                            reinterpret_cast<const Bytef*>(buffer.data()), buffer.size());
    if (result != Z_OK) {
        throw runtime_error("Decompression failed.");
    }
    decompressedContent.resize(decompressedSize);
    return decompressedContent;
}
string getBlobSHA(const string& filePath){
    ifstream inFile(filePath, ios::binary);
    if (!inFile) {
        cerr << "Error: Cannot open file: " << filePath << endl;
        return "";
    }

    stringstream buffer;
    buffer << inFile.rdbuf();
    string fileContent = buffer.str();
    inFile.close();

    return calculateFileSHA(fileContent);
}

void writeBlobObject(const string& filePath, const string& sha){
    string compressedContent = compressFileContent(filePath);
    if (compressedContent.empty()) {
        cerr << "Error: Failed to compress file content." << endl;
        return;
    }

    // Create the .mygit/objects directory if it doesn't exist
    string blobDir = "./.mygit/objects/";
    mkdir(blobDir.c_str(), 0777);  // Create directory if not present

    // Write the compressed content to the file named with the SHA-1 hash
    string blobPath = blobDir + sha;
    int fd = open(blobPath.c_str(), O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        cerr << "Error: Could not create blob file." << endl;
        return;
    }

    if (write(fd, compressedContent.c_str(), compressedContent.size()) < 0) {
        cerr << "Error: Could not write to blob file." << endl;
    }

    close(fd);
    cout << "Compressed file stored as blob: " << sha << endl;
}
void storeObject(const string& objectSHA, const string& objectContent) {
    string objectDir = "./.mygit/objects";
    string objectPath = "./.mygit/objects/" + objectSHA;
    if (!filesystem::exists(objectDir)) {
        filesystem::create_directories(objectDir);
    }
    ofstream outFile(objectPath, ios::binary);
    if (!outFile) {
        cerr << "Error: Could not write object file for " << objectSHA << endl;
        return;
    }
    outFile << objectContent;
    outFile.close();
}
string createTreeObject(const string& dirPath){
    stringstream treeContent;
    for(const auto& entry : fs::directory_iterator(dirPath)) {
        string filePath = entry.path().string();
        string fileName = entry.path().filename().string();
        string fileSHA;
        if (fileName == ".mygit") {
            continue;
        }
        if (entry.is_regular_file()){
            fileSHA = getBlobSHA(filePath);  
            treeContent << "100644 blob " << fileSHA << " " << fileName << "\n";
        } 
        else if (entry.is_directory()){
            fileSHA = createTreeObject(filePath);
            treeContent << "040000 tree " << fileSHA << " " << fileName << "/\n";
        }
    }
    string treeData = treeContent.str();
    string treeSHA = calculateFileSHA(treeData);  
    storeObject(treeSHA, treeData);
    return treeSHA;  
}

void writeTree(){
    string currentDir = ".";
    string treeSHA = createTreeObject(currentDir);
    if (!treeSHA.empty()) {
        cout << "Tree object SHA-1: " << treeSHA << endl;
    } else {
        cerr << "Error: Failed to create tree object." << endl;
    }
}
string getBlobPath(const string& fileSha){
    // Ensure SHA-1 hash is 40 characters long
    if (fileSha.size() != 40) {
        cerr << "Error: Invalid SHA-1 hash.\n";
        return "";
    }
    string blobDir = ".mygit/";         
    string blobPath = blobDir + "objects/" +  fileSha;    
    if (!fs::exists(blobDir)) {
        cerr << "Error: Blob directory does not exist.\n";
        return "";
    }
    if (!fs::exists(blobPath)) {
        cerr << "Error: Blob file not found.\n";
        return "";
    }
    return blobPath;
}
void parseTreeObject(const string& treeSha, bool nameOnly){
    string treePath = "./.mygit/objects/" + treeSha;
    ifstream treeFile(treePath, ios::binary);
    if (!treeFile) {
        cerr << "Error: Tree object not found for SHA-1: " << treeSha << endl;
        return;
    }
    string line;
    while (getline(treeFile, line)) {
        stringstream ss(line);
        string mode, type, sha, name;
        ss >> mode >> type >> sha >> name;
        if(nameOnly){
            cout << name << endl;
        } 
        else{
            cout << mode << " " << type << " " << sha << " " << name << endl;
        }
    }

    treeFile.close();
}

void getFileRecursive(vector<string> &st, string path, string dirName){
    DIR *dir;
    struct dirent *sd;
    // cout << "Inside dir: ";
    string fullPath = path + dirName;
    // cout << fullPath << endl;
    dir = opendir(fullPath.c_str());

    while ((sd = readdir(dir)) != NULL){
        string currFile = sd->d_name;
        if(currFile == ".." || currFile == "." || currFile == ".mygit" || currFile == "add.h" || currFile == "mygit" || currFile == ".git" || currFile == ".vscode" || currFile == "main.cpp" || currFile == "README.md"|| currFile == "Makefile")
            continue;
        else
        {
            struct stat sfile;
            currFile = path + dirName + "/" + sd->d_name;
            stat(currFile.c_str(), &sfile);
            // cout << currFile << endl;

            if (fs::is_directory(currFile.c_str()))
            {
                // cout << "recur " << dirName + "/" + sd->d_name << endl;
                getFileRecursive(st, path, dirName + "/" + sd->d_name);
            }
            else
                st.push_back(dirName + "/" + sd->d_name);
        }
    }
    // cout << "going out\n";
}
void changes_on_commit(){
    string vcspath1 = "./.mygit/tracked_current.txt";
    string vcspath2 = "./.mygit/tracked_history.txt";
    ifstream vfile1;
    ofstream vfile2;
    vfile1.open(vcspath1);
    vfile2.open(vcspath2, ios_base::in | ios_base::out | ios_base::app);
    if (!vfile1.is_open() && !vfile2.is_open()){
        perror("Error open");
        return;
    }
    string line;
    while (getline(vfile1, line)){
        vfile2 << line;
        vfile2 << "\n";
    }
    vfile1.close();
    vfile2.close();
    ofstream ofs;
    ofs.open(vcspath1, ofstream::out | ofstream::trunc);
    ofs.close();
}

void handleCommit(string commitMsg){
    if (versionNo > 0){
        string path1 = "./.mygit/" + to_string(versionNo - 1) + "/", path2 = "./.mygit/" + to_string(versionNo) + "/";
        DIR *dir1, *dir2, *dir;
        struct dirent *sd;
        dir1 = opendir(path1.c_str());
        dir2 = opendir(path2.c_str());
        if (dir1 == NULL || dir2 == NULL){
            cout << "'.mygit' folder corrupted.\n";
            return;
        }
        vector<string> old, latest; 
        dir = dir2;
        while((sd = readdir(dir)) != NULL){
            string currFile = sd->d_name;
            if (currFile == ".." || currFile == "." || currFile == ".mygit" || currFile == "add.h" || currFile == "mygit" || currFile == ".git" || currFile == ".vscode" || currFile == "main.cpp" || currFile == "README.md"|| currFile == "Makefile")
                continue;
            else{
                struct stat sfile;
                currFile = path2 + sd->d_name;
                stat(currFile.c_str(), &sfile);

                if (fs::is_directory(currFile.c_str()))
                    getFileRecursive(latest, path2, sd->d_name);
                else
                    latest.push_back(sd->d_name);
            }
        }
        if (latest.empty()){
            cout << "Nothing to commit..\n";
            cout << "Commit Message is: "<< commitMsg << "\n";
            return;
        }
        dir = dir1;
        while((sd = readdir(dir)) != NULL){
            string currFile = sd->d_name;
            if (currFile == ".." || currFile == "." || currFile == ".mygit" || currFile == "add.h" || currFile == "mygit" || currFile == ".git" || currFile == ".vscode" || currFile == "main.cpp" || currFile == "README.md"|| currFile == "Makefile")
                continue;
            else{
                struct stat sfile;
                currFile = path1 + currFile;
                stat(currFile.c_str(), &sfile);

                if (fs::is_directory(currFile.c_str()))
                    getFileRecursive(old, path1, sd->d_name);
                else
                    old.push_back(sd->d_name);
            }
        }
        for(auto it1 : old){
            auto it2 = find(latest.begin(), latest.end(), it1);
            if (it2 != latest.end())
                createForPatch(path1 + it1, path2 + *it2);
        }
    }
    versionNo++;
    string temp = "./.mygit/" + to_string(versionNo);
    if (mkdir(temp.c_str(), 0777) == -1){
        cerr << "Error creating new version folder:  " << strerror(errno) << endl;
        return;
    }
    string fileName = "./.mygit/version.txt";
    string hexCode;
    temp = to_string(versionNo);
    remove(fileName.c_str());
    fstream file(fileName.c_str(), ios::in | ios::app);
    file.write(temp.c_str(), temp.size());
    file.close();
    changes_on_commit();
    string currHex = generateHex();
    while(commitHexMap.find(currHex) != commitHexMap.end()) currHex = generateHex();

    string commitHex = calculateFileSHA(to_string(versionNo - 1) + commitMsg + currHex);
    auto t = chrono::system_clock::now();
    time_t currTime = chrono::system_clock::to_time_t(t);
    struct commitData cd;
    string timeStamp(ctime(&currTime));
    cd.commitNo = versionNo - 1;
    strcpy(cd.commitSHA, commitHex.c_str());
    strcpy(cd.message, commitMsg.c_str());
    strcpy(cd.timeStamp, timeStamp.c_str());
    strcpy(cd.hexCode, currHex.c_str());
    if (versionNo > 1){
        string parentSHA = commitInfoMap[versionNo - 2].commitSHA;
        strcpy(cd.parentcommitSHA, parentSHA.c_str());
    }
    commitHexMap[commitHex] = versionNo - 1;
    commitInfoMap[versionNo - 1] = cd;
    FILE *commitFile;
    commitFile = fopen("./.mygit/commit.txt", "a");
    if(!commitFile){
        cout << "Invalid file" << endl;
        return;
    }
    if(fwrite(&cd, sizeof(struct commitData), 1, commitFile) == 0){
        string msg = "commit File Handling Error!!\n";
        cout << msg << endl;
    }
    fclose(commitFile);
    cout << "\nCommit " << versionNo - 1 << " is added with " << commitHex << "\n";
    cout << "Commit Message is: "<< commitMsg << "\n";
}

void log(){
    int i = commitInfoMap.size()-1;
    for (auto it = commitInfoMap.rbegin(); it != commitInfoMap.rend(); it++){
        struct commitData cmt = it->second;
        cout << "\nCommit SHA: " << cmt.commitSHA;
        cout << "  with (Version : " << i << ")" << endl;
        if(i>0){
            cout << "Parent SHA: " << cmt.parentcommitSHA;
            cout << "  with (Version : " << i-1 << ")" << endl;
        }
        cout << "Commit message: \""<< cmt.message << "\"\n";
        cout << "Timestamp: " << cmt.timeStamp;
        cout << "Commit Id: " << cmt.hexCode<<endl;
        i--;
    }
}

vector<string> parseCommands(string cmnd){
    vector<string> args;
    char* cstr = new char[cmnd.length() + 1];
    strcpy(cstr, cmnd.c_str());
    char* token = strtok(cstr, " ");
    while (token != nullptr) {
        args.push_back(string(token));
        token = strtok(nullptr, " ");
    }
    delete[] cstr;
    return args;
}

bool getFileName(string &fileName){
    char resolved[2000];
    realpath(fileName.c_str(), resolved);
    string abs_path = string(resolved);
    string cwd = fs::current_path();

    if (abs_path.substr(0, cwd.size()) == cwd){
        fileName = abs_path.substr(cwd.size() + 1, abs_path.size());
        // cout<<fileName<<endl;
        return true;
    }
    else{
        return false;
    }
}

void restoreCommitFiles(const commitData& commit) {
    string commitPath = "./.mygit/" + to_string(commit.commitNo) + "/";
    for (const auto& entry : fs::directory_iterator(commitPath)) {
        string filePath = entry.path().string();
        string fileName = entry.path().filename().string();

        // Avoid copying `.mygit` and other version control files
        if (fileName == ".mygit" || fileName == "add.h" || fileName == "mygit" ||
            fileName == ".git" || fileName == ".vscode" || fileName == "main.cpp" || fileName == "README.md"|| fileName == "Makefile"){
            continue;
        }

        fs::copy(filePath, "./" + fileName, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
    }
}
void checkout(const string& commitSHA) {
    string fileName = "./.mygit/version.txt";
    int commitNo = -1;
    for(const auto& entry : commitInfoMap){
        if (entry.second.commitSHA == commitSHA) {
            commitNo = entry.first;
            break;
        }
    }
    if(commitNo == -1){
        cout << "Error: Commit SHA " << commitSHA << " does not exist." << endl;
        return;
    }

    for (const auto& entry : fs::directory_iterator("./")) {
        string currFile = entry.path().filename().string();
        if (currFile == ".." || currFile == "." || currFile == ".mygit" || currFile == "add.h" || 
            currFile == "commit.h" || currFile == "status.h" || currFile == "diff.h" || currFile == "mygit" || 
            currFile == ".git" || currFile == ".vscode" || currFile == "main.cpp" || currFile == "rollback.h" || 
            currFile == "checkout" || currFile == "README.md" || currFile == "Makefile") {
            continue;
        }
        fs::remove_all(entry.path());
    }

    for (int i = 0; i <= commitNo; i++) {
        if (commitInfoMap.find(i) != commitInfoMap.end()) {
            restoreCommitFiles(commitInfoMap[i]);
        }
    }
    vector<int> keysToDelete;
    for(const auto& entry : commitInfoMap){
        if(entry.first > commitNo){
            keysToDelete.push_back(entry.first);
        }
    }

    for(int key : keysToDelete){
        commitInfoMap.erase(key);
    }
    // cout<<"\nContents of commitInfoMap: \n";
    // for(const auto& entry : commitInfoMap){
    //     cout<<entry.first<<"\n";
    // }
    ofstream versionFile(fileName);
    versionFile << commitNo;
    versionFile.close();
    for(int i = commitNo + 1; i <= versionNo; i++){
        string path = "./.mygit/" + to_string(i);
        if(fs::exists(path)) {
            fs::remove_all(path);
        }
    }
    updateCommitInfoFile();
    cout << "Checked out to commit " << commitNo << " with SHA " << commitSHA << "." << endl;
}


int main(int argc, char *argv[]){
    bool writeBlob = false;
    string cmnd = "";
    for (int i = 0; i < argc; i++){
        string temp(argv[i]);
        cmnd += temp + " ";
    }
    cmnd = cmnd.substr(0, cmnd.size() - 1);

    checkVcs();

    vector<string> cmndArgs = parseCommands(cmnd);
    if(cmndArgs[0] != "./mygit"){
        cout << "Invalid first argument\n";
    }
    else if(cmndArgs.size()==2 && cmndArgs[1] == "init"){
        if(!vcs) handleInit(); // passing path as argument
        else  cout << "Mini Version Control System initialised already...\n";
    }
    else if (cmndArgs.size()==4 && cmndArgs[1] == "hash-object" && cmndArgs[2] == "-w"){
        if(!vcs) cout << "Mini Version Control System not initialised...\n";
        else{
            string filePath = cmndArgs[3];
            if (access(filePath.c_str(), F_OK) == -1){
                cerr << "Error: File does not exist or cannot be accessed.\n";
                return 1;
            }
            string sha = calculateFileSHA(filePath);
            if (sha.empty()) {
                cerr << "Error: Could not calculate SHA-1 for the file.\n";
                return 1;
            }
            cout << "SHA-1: " << sha << endl;
            writeBlobObject(filePath, sha);
        }
    }
    else if (cmndArgs.size()== 3 && cmndArgs[1] == "hash-object"){
        if(!vcs) cout << "Mini Version Control System not initialised...\n";
        else{
            string filePath = cmndArgs[2];
            if (access(filePath.c_str(), F_OK) == -1){
                cerr << "Error: File does not exist or cannot be accessed.\n";
                return 1;
            }
            string sha = calculateFileSHA(filePath);
            if (sha.empty()) {
                cerr << "Error: Could not calculate SHA-1 for the file.\n";
                return 1;
            }
            cout << "SHA-1: " << sha << endl;
        }
    }
    else if (cmndArgs.size() == 4 && cmndArgs[1] == "cat-file"){
        if(!vcs) cout << "Mini Version Control System not initialised...\n";
        else{
            string flag = cmndArgs[2];
            string fileSha = cmndArgs[3];
            string blobPath = getBlobPath(fileSha);
            if (blobPath.empty()) {
                cerr << "Error: Blob file not found for SHA-1: " << fileSha << endl;
                return 1;
            }
            if (flag == "-p"){
                string decompressedContent = decompressFileContent(blobPath);
                if (decompressedContent.empty()) {
                    cerr << "Error: Could not decompress the blob file.\n";
                    return 1;
                }
                cout << decompressedContent<<"\n";
            } 
            else if (flag == "-s"){
                string decompressedContent = decompressFileContent(blobPath);
                cout << "File size: " << decompressedContent.size() << " bytes" << endl;
            } 
            // complete this
            else if (flag == "-t") {
                // Flag '-t': Show the object type
                // For simplicity, we assume all files stored are "blob" objects
                cout << "Object type: blob" << endl;
            } 
            else {
                cerr << "Error: Invalid flag provided. Use -p, -s, or -t.\n";
                return 1;
            }
        }
    }
    else if( cmndArgs.size()==2 && cmndArgs[1] == "write-tree"){
        if(!vcs) cout << "Mini Version Control System not initialised...\n";
        else{
            writeTree();
        }
    }
    else if( cmndArgs.size()==4 && cmndArgs[1] == "ls-tree" && cmndArgs[2] == "--name-only"){
        if(!vcs) cout << "Mini Version Control System not initialised...\n";
        else{
            string treeSha = cmndArgs[3];
            parseTreeObject(treeSha, true);
        }
    }
    else if( cmndArgs.size()==3 && cmndArgs[1] == "ls-tree"){
        if(!vcs) cout << "Mini Version Control System not initialised...\n";
        else{
            string treeSha = cmndArgs[2];
            parseTreeObject(treeSha, false);
        }
    }
    else if (!vcs && cmndArgs[1] == "commit" && cmndArgs.size() > 1){
        cout<<"Mini Version Control System not initialized\n";
        return 0;
    }
    else if (vcs && cmndArgs[1] == "commit"){
        if(cmndArgs.size() > 2 && cmndArgs[2] == "-m"){
            string commitMsg = "";
            for (int i = 3; i < (int)cmndArgs.size(); i++)
                commitMsg += cmndArgs[i] + " ";
            handleCommit(commitMsg.substr(0, commitMsg.size() - 1)); // passing path as argument
            cout << "Commit command run successfully!\n";
        }
        else if(cmndArgs.size() == 2 ){
            string commitMsg = "Default commit message";
            handleCommit(commitMsg); // passing path as argument
            cout << "Commit command run successfully!\n";
        }
        else{
            cout<<"Invalid Command!\n";
            return 0;
        }
    }
    else if (!vcs && cmndArgs[1] == "add" && cmndArgs.size() > 1){
        cout<<"Mini Version Control System not initialized\n";
        return 0;
    }
    else if (vcs && cmndArgs[1] == "add" && cmndArgs.size() > 1){
        bool msg = true;
        for (int i = 2; i < (int)cmndArgs.size(); i++){
            if (fs::exists(cmndArgs[i]) == false){
                cout << "File " << cmndArgs[i] << " not present...\n";
                msg = false;
                continue;
            }
            if (cmndArgs[i] == "." || getFileName(cmndArgs[i])){
                add::addComplete(cmndArgs[i]);
                cout << "Adding " << cmndArgs[i] << endl;
            }
            else{
                cout << "File " << cmndArgs[i] << " is outside of cwd\n";
                msg = false;
                continue;
            }
        }
        if (msg){
            cout << "All the file/s are added successfully!\n";
        }
    }
    else if (vcs && cmndArgs[1] == "checkout" && cmndArgs.size() == 3){
        if(!vcs){
            cout<<"Mini Version Control System not initialized\n";
            return 0;
        }
        else checkout(cmndArgs[2]);
    }
    else if(!vcs && cmndArgs[1] == "log"){
        cout<<"Mini Version Control System not initialized\n";
        return 0;
    }
    else if(vcs && cmndArgs[1] == "log"){
        log();
    }
    else{
        cout<<"Invalid Command!\n";
        return 0;
    }
    cout << endl;
    return 0;
}
