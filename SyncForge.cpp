#include <filesystem>
#include <iostream>
#include<vector>
#include<string>
#include <algorithm>
#include <fstream>
#include <sstream>
#include "sha1.h"
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
	int largestCommonSubsequence(std::vector<std::string> &file1, std::vector<std::string> &file2, int m, int n, std::vector<std::vector<int>> &dp){

		if(m==0 || n==0) return 0; //Base Case

		if (dp[m][n] != -1) return dp[m][n]; // Already exists in the dp table
		//match
		if(file1[m-1] == file2[n-1]){
			return dp[m][n] = 1 + largestCommonSubsequence(file1, file2, m-1, n-1, dp);
		}

		//not match
		return dp[m][n] = std::max(largestCommonSubsequence(file1, file2, m-1, n, dp), largestCommonSubsequence(file1, file2, m, n-1, dp));
	}
	std::vector<std::string> findLCSfromDP(std::vector<std::string> &file2, int m, int n, std::vector<std::vector<int>> &dp){
		std::vector<std::string> res;
		int i=m;
		for(int j = n; j > 0; j--){
			if(dp[i][j] == dp[i][j-1]){
				continue;
			}
			else{
				res.push_back(file2[j-1]);
				i--;
			}
		}

		reverse(res.begin(), res.end());

		return res;
	}

	std::vector<std::string> find_diff(std::vector<std::string> file1, std::vector<std::string> file2){
		std::vector<std::string> res;
		int m = file1.size();
		int n = file2.size();

		if(m < n) {
			return {"inavlid"};
		}

		int i = 0, j = 0;

		while(j < n){
			if(file1[i] == file2[j]){
				i++;
				j++;
			}else{
			res.push_back(file1[i]);
			i++;
			}
		}

		while(i < m){
			res.push_back(file1[i]);
			i++;
		}

		return res;
	}
	commitData getCommitData(std::string comitHash){
		std::ifstream file(objectsPath/comitHash, std::ios::binary);
		if(file.is_open()){
			std::string data;
			std::getline(file, data);
			file.close();
			return commitData::deserialize(data);
		}
		else{
			std::cerr << "Error in Opening of ComitHash File\n";
		}
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
	void showCommitDiff(std::string comitHash){
		commitData data = getCommitData(comitHash);
		std::cerr << "Changes in the Last Commit are:-\n";
		if(data.parent!=""){
			commitData parentData = getCommitData(data.parent);
			for(auto file : data.files){
				std::cerr << "\nFile Name : "<<file.filePath<<"\n\n";
				std::ifstream fs(objectsPath/file.hash);
				std::vector<std::string> v1,v2;
				if(fs.is_open()){
					std::string line;
					while(std::getline(fs, line)){
						v2.push_back(line);
					}
					fs.close();
				}
				else{
					std::cerr << "Error opening file!\n";
				}
				for(auto f:parentData.files){
					if(f.filePath==file.filePath){
						fs.open(objectsPath/f.hash);
						if(fs.is_open()){
							std::string line;
							while(std::getline(fs, line)){
								v1.push_back(line);
							}
							fs.close();
						}
						else{
							std::cerr << "Error opening file!\n";
						}
						break;
					}
				}

				int m=v1.size(), n=v2.size();
				std::vector<std::vector<int>> dp(m+1, std::vector<int>(n+1));
				for(int i=0; i<=m; i++) for(int j=0; j<=n; j++) dp[i][j]=-1;
				int lcslen = largestCommonSubsequence(v1, v2, m, n, dp);
				std::vector<std::string> lcs = findLCSfromDP(v2, m, n, dp);

				int i=0, j=0, k=0;
				while(j<lcs.size()){
					while(v1[i]!=lcs[j]) std::cerr<<"\033[1;31m-- : "<<v1[i++]<<"\033[0m\n";
					while(v2[k]!=lcs[j]) std::cerr<<"\033[1;32m++ : "<<v2[k++]<<"\033[0m\n";
					std::cerr<<"   : "<<lcs[j++]<<"\n";
					k++;
					i++;
				}
				while(i<m) std::cerr<<"\033[1;31m-- : "<<v1[i++]<<"\033[0m\n";
				while(k<n) std::cerr<<"\033[1;32m++ : "<<v2[k++]<<"\033[0m\n";
			}
		}
		else{
			for(auto file : data.files){
				std::cerr << "\nFile Name : "<<file.filePath<<"\n\n";
				std::ifstream fs(objectsPath/file.hash);
				std::vector<std::string> v1,v2;
				if(fs.is_open()){
					std::string line;
					while(std::getline(fs, line)){
						std::cerr<<"\033[1;32m++ : "<<line<<"\033[0m\n";
					}
					fs.close();
				}
				else{
					std::cerr << "Error opening file!\n";
				}
			}
		}
	}
};

int main(int argc, char* argv[]) {
    SyncForge syncforge;

    if (argc < 2) {
        std::cerr << "No command provided!\n";
        return 1;
    }

    std::string command = argv[1];

    if (command == "init") {
        syncforge = SyncForge();
        std::cerr << "Initialized empty SyncForge repository\n";
    } 
    else if (command == "add" && argc == 3) {
        std::string fileToAdd = argv[2];
        syncforge.add(fileToAdd);
    } 
    else if (command == "commit" && argc == 4) {
        std::string message = argv[3];
        syncforge.commit(message);
    } 
    else if (command == "log") {
        syncforge.log();
    } 
    else if (command == "diff" && argc == 3) {
        std::string commitHash = argv[2];
        syncforge.showCommitDiff(commitHash);
    } 
    else {
        std::cerr << "Unknown command or wrong number of arguments\n";
    }

    return 0;
}
