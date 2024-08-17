#include <filesystem>
#include <iostream>
#include<vector>
#include<string>
#include <fstream>
#include <sstream>
#include "sha1.h"
#include <utility>
#include <ctime> 
using namespace std::filesystem;

class SyncForge{
private:
	path repoPath,indexPath,objectsPath,headPath;
	struct index{
		std::string hash;
		std::string filePath;
		std::string serialize() const {
		return filePath +","+ hash + "\n";
		}
		static index deserialize(std::string &data){
			index i;
			std::stringstream ss(data);
			std::getline(ss, i.filePath, ',');
			std::getline(ss, i.hash);
			return i;
		}
	};
	struct commitData{
		std::string time;
		std::string message;
		std::vector<index> files;
		std::string parent;
		std::string serialize() const {
		std::ostringstream oss;
		oss << time <<"," << message <<"," << parent <<",";
		
		for (const auto& file : files) {
			oss << file.filePath<<","<<file.hash<<",";
		}
		oss<<"\n";
		return oss.str();
		}
		static commitData deserialize(std::string &data){
			commitData cd;
			std::istringstream iss(data);

			std::getline(iss, cd.time, ',');
			std::getline(iss, cd.message, ',');
			std::getline(iss, cd.parent, ',');
			
			while(iss.good()){
				index i;
				getline(iss, i.filePath, ',');
				getline(iss, i.hash, ',');
				if (!i.filePath.empty() && !i.hash.empty()) {
					cd.files.push_back(i);
				}
			}
			return cd;
		}
	}; 
	void init(){
		try{
			create_directory(repoPath);
			create_directory(objectsPath);
		}
		catch (const filesystem_error& e) {
			std::cerr << "Error creating directories: " << e.what() << "\n";
		}
		if(exists(headPath) || exists(indexPath)){
			std::cerr<<"Already Initialise the SyncForge"<<"\n";
		}
		else{
			std::ofstream headFile(headPath, std::ios::out | std::ios::trunc);
			if (headFile.is_open()) {
				headFile << "";
				headFile.close();
			} else {
				std::cerr << "Unable to open Head file for writing.\n";
			}
			std::ofstream indexFile(indexPath, std::ios::out | std::ios::trunc);
			if (indexFile.is_open()) {
				indexFile << "";
				indexFile.close();
			} else {
				std::cerr << "Unable to open Index file for writing.\n";
			}
		}
	}
	void updateStagingArea(std::string filePath, std::string hash){
		index i;
		i.filePath = filePath;
		i.hash = hash;
		std::ofstream fs(indexPath, std::ios::binary | std::ios::app);
		if (fs.is_open()) {
			fs << i.serialize();
			fs.close();
		} else {
			std::cerr << "Error opening file!" << std::endl;
		}
	}
	std::string getCurrentHead(){
		std::string data;
		std::ifstream  file(headPath);
		if(file.is_open()){
			file>>data;
			file.close();
		}
		return data;
	}

public:
	SyncForge(){
		this->repoPath = (current_path());
		this->repoPath = this->repoPath / ".SyncForge";
		this->objectsPath = this->repoPath / "objects";
		this->headPath = this->repoPath / "Head";
		this->indexPath = this->repoPath / "index";
		this->init();
	}
	void add(std::string fileToBeAdded){
		std::ifstream file(current_path()/fileToBeAdded);
		std::string content="";
		std::string myline;
		if(file.is_open()){
			while(std::getline (file, myline)){
				content+=myline;
				content+="\n";
			}
			file.close();
		}
		
		std::string hash = sha1(content); //hash of the file
		std::ofstream hashedFile(objectsPath/hash, std::ios::out | std::ios::binary);
		if (hashedFile.is_open()) {
			hashedFile << content; //writing content in hash object file
			hashedFile.close();
		} 
		updateStagingArea(fileToBeAdded, hash);
		std::cerr<<"Added "<<fileToBeAdded<<"\n";
	}
	void commit(std::string message){
		commitData data;
		time_t t = std::time(nullptr);
		data.time = std::ctime(&t);
		data.time.pop_back(); 
		data.message = message;
		std::ifstream ifs(indexPath, std::ios::binary);
		if (ifs.is_open()) {
			std::string line;
			while(std::getline(ifs, line)){
				data.files.push_back(index::deserialize(line));
			}
			ifs.close();
		} else {
			std::cerr << "Error opening file!\n";
		}
		data.parent = getCurrentHead();
		
		std::string comitHash = sha1(data.serialize());
		std::ofstream fs(objectsPath/comitHash, std::ios::out | std::ios::binary);
		if (fs.is_open()) {
			fs << data.serialize();
			fs.close();
		}
		std::ofstream fsys(headPath, std::ios::out | std::ios::binary);
		if (fsys.is_open()) {
			fsys << comitHash; //update head to point the new commit
			fsys.close();
		}
		fsys.open(indexPath, std::ofstream::out | std::ofstream::trunc);
		fsys.close();
		std::cerr<<"Commit SuccessFully Created: "<<comitHash<<"\n";
	}
	void log(){
		std::string currentCommitHash = getCurrentHead();
		while(currentCommitHash!=""){
			std::ifstream file(objectsPath/currentCommitHash, std::ios::binary);
			std::string line;
			std::getline(file, line);
			commitData data = commitData::deserialize(line);
			std::cerr<<"____________________________________________________\n";
			std::cerr<<"commit: "<<currentCommitHash<<"\n\n";
			std::cerr<<"Date: "<<data.time<<"\n\n";
			std::cerr<<"message: "<<data.message<<"\n";
			std::cerr<<"____________________________________________________\n";
			currentCommitHash = data.parent;
		}
	}
};

int main(int argc, char* argv[]) {
    SyncForge S;

    if (argc > 1) {
        std::string command = argv[1];

        if (command == "add" && argc == 3) {
            std::string fileToAdd = argv[2];
            S.add(fileToAdd);
        } else if (command == "commit" && argc == 3) {
            std::string message = argv[2];
            S.commit(message);
        } else if (command == "log") {
            S.log();
        } else {
            std::cerr << "Unknown command or incorrect arguments.\n";
        }
    } else {
        std::cerr << "Please provide a command.\n";
    }

    return 0;
}