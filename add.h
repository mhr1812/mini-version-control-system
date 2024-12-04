#include <iostream>
#include <dirent.h>
#include <filesystem>
#include <string>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <dirent.h>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utility>
#include <openssl/sha.h>

namespace add{
    //tracked_current, tracked_history
    using namespace std;
    void copydirectory(string path1, string path2);
    void copyfile(string path1, string path2);

    namespace fs = filesystem;
    string vcspath = "./.mygit"; // add .mygit
    
    void calculateFileSHA(string fileLoc){
        unsigned char completeFileSHA[SHA_DIGEST_LENGTH];
        FILE *inFile = fopen(fileLoc.c_str(), "rb");
        SHA_CTX shaContext;
        int bytes;
        // 524288
        unsigned char data[524288];

        if(inFile == NULL){
            cout << "Cannot open " << fileLoc << endl;
            return;
        }
        SHA1_Init(&shaContext);
        while((bytes = fread(data, 1, 524288, inFile)) != 0){
            SHA1_Update(&shaContext, data, bytes);
        }
        SHA1_Final(completeFileSHA, &shaContext);
        fclose(inFile);
        stringstream ss;
        for (int i = 0; i < SHA_DIGEST_LENGTH; i++){
            ss << hex << setw(2) << setfill('0') << (int)completeFileSHA[i];
        }
        string fullFileSHA1 = ss.str();
        // cout << fileLoc << " sha is " << fullFileSHA1 << endl;
        string appendData = fileLoc + "$" + fullFileSHA1 + "\n";
        string cwd = fs::current_path();
        string version;
        ofstream vfile;
        vfile.open("./.mygit/tracked_current.txt", ios_base::app); // append instead of overwrite
        vfile << appendData;
        vfile.close();
    }


    int createdir(string path1)
    {
        int check;
        check = mkdir(path1.c_str(), 0777);
        if (check == 0)
            return 1;
        else
            return -1;
    }
    void copy_version(string path)
    {
        string cwd = fs::current_path();
        cwd += "/"+path;
        string version;
        ifstream vfile("./.mygit/version.txt");
        vfile >> version;
        vfile.close();
        DIR *folder = opendir(cwd.c_str());
        if (folder == NULL)
            return;
        struct dirent *file = readdir(folder);
        while (file != NULL) {
            if (strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0 || strcmp(file->d_name, ".mygit") == 0 || strcmp(file->d_name, ".git") == 0 || strcmp(file->d_name, ".vscode") == 0 || strcmp(file->d_name, "main.cpp") == 0
            || strcmp(file->d_name, "add.h") == 0 || strcmp(file->d_name, "rollback.h") == 0 || strcmp(file->d_name, "commit.h") == 0 || strcmp(file->d_name, "diff.h") == 0 || strcmp(file->d_name, "status.h") == 0 || strcmp(file->d_name, "vcs") == 0){
                
            }
            else
            {

                string filepath = cwd + "/" + file->d_name;
                string new_path = "./.mygit/" + version + "/"+path+"/" + string(file->d_name);
                if (file->d_type == DT_DIR)
                {
                    copydirectory(filepath, new_path);
                }

                else
                {
                    copyfile(filepath, path+"/" + string(file->d_name));
                }
            }
            file = readdir(folder);
        }
        closedir(folder);
    }
    void create_dir_structure(string check_path, string version)
    {
        string cwd = fs::current_path();
        char resolved[2000];
        realpath(check_path.c_str(), resolved);
        // cout<<
        string abs_path = string(resolved);
        vector<string> directories_in_path;
        string temp;
        for (int i = cwd.size() + 1; i < (int)abs_path.size(); i++)
        {
            if (abs_path[i] != '/')
            {
                temp += abs_path[i];
            }
            else
            {
                directories_in_path.push_back(temp);
                temp = "";
            }
        }
        string temp_path = vcspath + "/" + version;
        for (auto i : directories_in_path)
        {
            temp_path = temp_path + "/" + i;
            if (fs::exists(temp_path))
            {
                continue;
            }
            else
            {
                string create_command = "mkdir " + temp_path;
                system(create_command.c_str());
            }
        }
    }
    void copyfile(string path1, string filePath)
    {
        string version, path2 = filePath;
        ifstream vfile("./.mygit/version.txt");
        vfile >> version;
        vfile.close();

        if(filePath.substr(0, 7) != "./.mygit/"){
            path2 = "./.mygit/" + version + "/"+filePath;
        }
        string cwd = fs::current_path();
        calculateFileSHA(path1);
        char buffer[1000];
        memset(&buffer[0], 0, sizeof(buffer));
        int filecheck1, filecheck2;
        struct stat buff;
        stat(path1.c_str(), &buff);
        filecheck1 = open(path1.c_str(), O_RDONLY);
        if(filecheck1 == -1){
            printf("Error opening first_file\n");
            close(filecheck1);
            return;
        }
        filecheck2 = open(path2.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if(filecheck2 == -1){
            create_dir_structure(path1, version);
            filecheck2 = open(path2.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        }
        int a = 0;
        while((a = read(filecheck1, buffer, 1000))){
            if(a != -1){
                write(filecheck2, buffer, a);
            }
            else break;
        }
        chmod(path2.c_str(), buff.st_mode);
        close(filecheck1);
        close(filecheck2);
    }
    void copydirectory(string path1, string path2){
        string cwd = fs::current_path();
        struct stat mbuff;
        stat(path1.c_str(), &mbuff);
         bool isEmpty = true;
        for (const auto& entry : fs::directory_iterator(path1)) {
            isEmpty = false;
            break;
        }
        if (isEmpty) {
            return; // Skip empty directories
        }
        DIR *folder = opendir(path1.c_str());
        if (!(fs::exists(path2.c_str()))){
            string version;
            ifstream vfile("./.mygit/version.txt");
            vfile >> version;
            vfile.close();
            create_dir_structure(path1, version);
        }

        if(folder == NULL){
            cout << "returning null";
            return;
        }
        struct dirent *file;
        file = readdir(folder);
        while(file != NULL){
            if(strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0){
            }
            else{
                // string filepath= path1 +"/"+file->d_name;
                if(file->d_type == DT_DIR){
                    // cout << "Directory " << path1 + "/" + file->d_name << endl;
                    pair<string, string> PAIR;
                    PAIR.first = path1 + "/" + file->d_name;
                    PAIR.second = path2 + "/" + file->d_name;
                    struct stat buff;
                    stat(path1.c_str(), &buff);
                    copydirectory(PAIR.first, PAIR.second);
                }
                else{
                    pair<string, string> PAIR;
                    PAIR.first = path1 + "/" + file->d_name;
                    PAIR.second = path2 + "/" + file->d_name;
                    struct stat buff;
                    stat(path1.c_str(), &buff);
                    copyfile(PAIR.first, PAIR.second);
                }
            }
            file = readdir(folder);
            chmod(path2.c_str(), mbuff.st_mode);
        }
        closedir(folder);
    }

    void addComplete(string path){
        string cwd = fs::current_path();
        string filepath1 = cwd + "/" + path;
        if(fs::is_directory(filepath1.c_str())){
            copy_version(path);
        } 
        else{
            copyfile(filepath1,path);
        }
    }
}
