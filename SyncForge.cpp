#include <filesystem>
#include <iostream>
#include<vector>
#include<string>
#include <algorithm>
#include <fstream>
#include <sstream>
#include "sha1.h"
#include <ctime> 
using namespace std;
using namespace filesystem;

class SyncForge{
private:
	path repoPath,indexPath,objectsPath,headPath;
	struct index{
		string hash;
		string filePath;
		string serialize() const {
		return filePath +","+ hash + "\n";
		}
		static index deserialize(string &data){
			index i;
			stringstream ss(data);
			getline(ss, i.filePath, ',');
			getline(ss, i.hash);
			return i;
		}
	};
	struct commitData{
		string time;
		string message;
		vector<index> files;
		string parent;
		string serialize() const {
		ostringstream oss;
		oss << time <<"," << message <<"," << parent <<",";
		
		for (const auto& file : files) {
			oss << file.filePath<<","<<file.hash<<",";
		}
		oss<<"\n";
		return oss.str();
		}
		static commitData deserialize(string &data){
			commitData cd;
			istringstream iss(data);

			getline(iss, cd.time, ',');
			getline(iss, cd.message, ',');
			getline(iss, cd.parent, ',');
			
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
			cerr << "Error creating directories: " << e.what() << "\n";
		}
		if(exists(headPath) || exists(indexPath)){
			cerr<<"Already Initialise the SyncForge"<<"\n";
		}
		else{
			ofstream headFile(headPath, ios::out | ios::trunc);
			if (headFile.is_open()) {
				headFile << "";
				headFile.close();
			} else {
				cerr << "Unable to open Head file for writing.\n";
			}
			ofstream indexFile(indexPath, ios::out | ios::trunc);
			if (indexFile.is_open()) {
				indexFile << "";
				indexFile.close();
			} else {
				cerr << "Unable to open Index file for writing.\n";
			}
		}
	}
	void updateStagingArea(string filePath, string hash){
		index i;
		i.filePath = filePath;
		i.hash = hash;
		ofstream fs(indexPath, ios::binary | ios::app);
		if (fs.is_open()) {
			fs << i.serialize();
			fs.close();
		} else {
			cerr << "Error opening file!" << endl;
		}
	}
	string getCurrentHead(){
		string data;
		ifstream  file(headPath);
		if(file.is_open()){
			file>>data;
			file.close();
		}
		return data;
	}
	int largestCommonSubsequence(vector<string> &file1, vector<string> &file2, int m, int n, vector<vector<int>> &dp){

		if(m==0 || n==0) return 0; //Base Case

		if (dp[m][n] != -1) return dp[m][n]; // Already exists in the dp table
		//match
		if(file1[m-1] == file2[n-1]){
			return dp[m][n] = 1 + largestCommonSubsequence(file1, file2, m-1, n-1, dp);
		}

		//not match
		return dp[m][n] = max(largestCommonSubsequence(file1, file2, m-1, n, dp), largestCommonSubsequence(file1, file2, m, n-1, dp));
	}
	vector<string> findLCSfromDP(vector<string> &file2, int m, int n, vector<vector<int>> &dp){
		vector<string> res;
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

	vector<string> find_diff(vector<string> file1, vector<string> file2){
		vector<string> res;
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
	commitData getCommitData(string comitHash){
		ifstream file(objectsPath/comitHash, ios::binary);
		string data;
		if(file.is_open()){
			getline(file, data);
			file.close();
		}
		else{
			cerr << "Error in Opening of ComitHash File\n";
		}
		return commitData::deserialize(data);
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
	void add(string fileToBeAdded){
		ifstream file(current_path()/fileToBeAdded);
		string content="";
		string myline;
		if(file.is_open()){
			while(getline (file, myline)){
				content+=myline;
				content+="\n";
			}
			file.close();
		}
		
		string hash = sha1(content); //hash of the file
		ofstream hashedFile(objectsPath/hash, ios::out | ios::binary);
		if (hashedFile.is_open()) {
			hashedFile << content; //writing content in hash object file
			hashedFile.close();
		} 
		updateStagingArea(fileToBeAdded, hash);
		cerr<<"Added "<<fileToBeAdded<<"\n";
	}
	void commit(string message){
		commitData data;
		time_t t = time(nullptr);
		data.time = ctime(&t);
		data.time.pop_back(); 
		data.message = message;
		ifstream ifs(indexPath, ios::binary);
		if (ifs.is_open()) {
			string line;
			while(getline(ifs, line)){
				data.files.push_back(index::deserialize(line));
			}
			ifs.close();
		} else {
			cerr << "Error opening file!\n";
		}
		data.parent = getCurrentHead();
		
		string comitHash = sha1(data.serialize());
		ofstream fs(objectsPath/comitHash, ios::out | ios::binary);
		if (fs.is_open()) {
			fs << data.serialize();
			fs.close();
		}
		ofstream fsys(headPath, ios::out | ios::binary);
		if (fsys.is_open()) {
			fsys << comitHash; //update head to point the new commit
			fsys.close();
		}
		fsys.open(indexPath, ofstream::out | ofstream::trunc);
		fsys.close();
		cerr<<"Commit SuccessFully Created: "<<comitHash<<"\n";
	}
	void log(){
		string currentCommitHash = getCurrentHead();
		while(currentCommitHash!=""){
			ifstream file(objectsPath/currentCommitHash, ios::binary);
			string line;
			getline(file, line);
			commitData data = commitData::deserialize(line);
			cerr<<"____________________________________________________\n";
			cerr<<"commit: "<<currentCommitHash<<"\n\n";
			cerr<<"Date: "<<data.time<<"\n\n";
			cerr<<"message: "<<data.message<<"\n";
			cerr<<"____________________________________________________\n";
			currentCommitHash = data.parent;
		}
	}
	void showCommitDiff(string comitHash){
		commitData data = getCommitData(comitHash);
		cerr << "Changes in the Last Commit are:-\n";
		if(data.parent!=""){
			commitData parentData = getCommitData(data.parent);
			for(auto file : data.files){
				cerr << "\nFile Name : "<<file.filePath<<"\n\n";
				ifstream fs(objectsPath/file.hash);
				vector<string> v1,v2;
				if(fs.is_open()){
					string line;
					while(getline(fs, line)){
						v2.push_back(line);
					}
					fs.close();
				}
				else{
					cerr << "Error opening file!\n";
				}
				for(auto f:parentData.files){
					if(f.filePath==file.filePath){
						fs.open(objectsPath/f.hash);
						if(fs.is_open()){
							string line;
							while(getline(fs, line)){
								v1.push_back(line);
							}
							fs.close();
						}
						else{
							cerr << "Error opening file!\n";
						}
						break;
					}
				}

				int m=v1.size(), n=v2.size();
				vector<vector<int>> dp(m+1, vector<int>(n+1));
				for(int i=0; i<=m; i++) for(int j=0; j<=n; j++) dp[i][j]=-1;
				int lcslen = largestCommonSubsequence(v1, v2, m, n, dp);
				vector<string> lcs = findLCSfromDP(v2, m, n, dp);

				int i=0, j=0, k=0;
				while(j<lcs.size()){
					while(v1[i]!=lcs[j]) cerr<<"\033[1;31m-- : "<<v1[i++]<<"\033[0m\n";
					while(v2[k]!=lcs[j]) cerr<<"\033[1;32m++ : "<<v2[k++]<<"\033[0m\n";
					cerr<<"   : "<<lcs[j++]<<"\n";
					k++;
					i++;
				}
				while(i<m) cerr<<"\033[1;31m-- : "<<v1[i++]<<"\033[0m\n";
				while(k<n) cerr<<"\033[1;32m++ : "<<v2[k++]<<"\033[0m\n";
			}
		}
		else{
			for(auto file : data.files){
				cerr << "\nFile Name : "<<file.filePath<<"\n\n";
				ifstream fs(objectsPath/file.hash);
				vector<string> v1,v2;
				if(fs.is_open()){
					string line;
					while(getline(fs, line)){
						cerr<<"\033[1;32m++ : "<<line<<"\033[0m\n";
					}
					fs.close();
				}
				else{
					cerr << "Error opening file!\n";
				}
			}
		}
	}
};

int main(int argc, char* argv[]) {
    SyncForge syncforge;

    if (argc < 2) {
        cerr << "No command provided!\n";
        return 1;
    }

    string command = argv[1];

    if (command == "init") {
        syncforge = SyncForge();
        cerr << "Initialized empty SyncForge repository\n";
    } 
    else if (command == "add" && argc == 3) {
        string fileToAdd = argv[2];
        syncforge.add(fileToAdd);
    } 
    else if (command == "commit" && argc == 4) {
        string message = argv[3];
        if(argv[2] != "-m"){
        	cerr << "Missing Parameter -m ";
        	return 1;
        }
        syncforge.commit(message);
    } 
    else if (command == "log") {
        syncforge.log();
    } 
    else if (command == "diff" && argc == 3) {
        string commitHash = argv[2];
        syncforge.showCommitDiff(commitHash);
    } 
    else if (command == "help") {
        cerr << "SyncForge Commands:\n";
        cerr << "  init                 Initialize a new SyncForge repository\n";
        cerr << "  add <file>           Add a file to the staging area\n";
        cerr << "  commit -m <message>  Commit the staged files with a message\n";
        cerr << "  log                  Show the commit history\n";
        cerr << "  diff <hash>          Show the difference of a commit\n";
    } 
    else {
        cerr << "Unknown command or wrong number of arguments\n";
    }

    return 0;
}
