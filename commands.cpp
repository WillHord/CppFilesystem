// $Id: commands.cpp,v 1.27 2022-01-28 18:11:56-08 - - $

#include "commands.h"
#include "debug.h"
#include <iomanip>


vector<string> splitPath(string pathString){
   vector<string> paths;
   size_t val = 0;
   string path;
   while((val = pathString.find("/")) != string::npos){
      path = pathString.substr(0,val);
      if(path != "") paths.push_back(path);
      pathString.erase(0, val+1);
   }
   paths.push_back(pathString);
   return paths;
}

void printwords(const wordvec& words){
   cout << "words: ";
   for(auto i: words){
      cout << '|' << i << '|';
   }
   cout << endl;
}

void printState(inode_state& state){
   cout << "State: " << state.prompt() << endl;
}

const command_hash cmd_hash {
   {"cat"   , fn_cat   },
   {"cd"    , fn_cd    },
   {"echo"  , fn_echo  },
   {"exit"  , fn_exit  },
   {"ls"    , fn_ls    },
   {"lsr"   , fn_lsr   },
   {"make"  , fn_make  },
   {"mkdir" , fn_mkdir },
   {"prompt", fn_prompt},
   {"pwd"   , fn_pwd   },
   {"rm"    , fn_rm    },
   {"rmr"   , fn_rmr   },
};

command_fn find_command_fn (const string& cmd) {
   // Note: value_type is pair<const key_type, mapped_type>
   // So: iterator->first is key_type (string)
   // So: iterator->second is mapped_type (command_fn)
   DEBUGF ('c', "[" << cmd << "]");
   const auto result {cmd_hash.find (cmd)};
   if (result == cmd_hash.end()) {
      throw command_error (cmd + ": no such command");
   }
   return result->second;
}

command_error::command_error (const string& what):
            runtime_error (what) {
}

int exit_status_message() {
   int status {exec::status()};
   cout << exec::execname() << ": exit(" << status << ")" << endl;
   return status;
}


void fn_cat (inode_state& state, const wordvec& words) {
   DEBUGF ('c', state);
   DEBUGF ('c', words);

   inode_ptr initial_cwd = state.get_cwd();
   for(int k=1; k < static_cast<int>(words.size()); k++){
      if(words.size() != 1){
         if(words[k][0] == '/'){
            state.set_cwd(state.get_root());
         }

         vector<string> paths = splitPath(words[k]);
         inode_ptr newDir;
         for(int i=0; i < static_cast<int>(paths.size()); i++){
            directory_entries dirents = state.get_cwd()->get_dirents();
            if(dirents.find(paths[i]) != dirents.end()){
               if(i == static_cast<int>(paths.size()) - 1){
                  if(dirents.at(paths[i])->getFilePtr()->isDirectory() 
                     == "directory"){
                     throw command_error("cat: " + paths[i] + 
                        ": is a directory");
                  }
                  wordvec data = dirents.at(paths[i])->
                     getFilePtr()->readfile();
                  for(auto j: data){
                     cout << j << " ";
                  }
                  
               }
               state.set_cwd(dirents.at(paths[i]));
            } else {
               state.set_cwd(initial_cwd);
               throw command_error("cat: " + words[k] +
                  ": No such file or directory");
            }
         }
      }
      state.set_cwd(initial_cwd);
   }
   cout << endl;
}

void fn_cd (inode_state& state, const wordvec& words) {
   DEBUGF ('c', state);
   DEBUGF ('c', words);

   if(words.size() == 1){
      state.set_cwd(state.get_root());
      state.reset_pwd();
      return;
   }

   inode_ptr initial_cwd = state.get_cwd();
   vector<string> initial_pwd = state.get_pwd();
   if(words[1][0] == '/'){
      state.set_cwd(state.get_root());
      state.reset_pwd();
   }

   vector<string> paths = splitPath(words[1]);
   for(auto i: paths){
      directory_entries dirents = state.get_cwd()->get_dirents();
      if(dirents.find(i) != dirents.end()){
         state.set_cwd(dirents.at(i));
         if(i == ".." and state.get_pwd() != vector<string>{"/"}){
            state.remove_pwd();
         } else if(i == "."){
            continue;
         } else {
            state.add_pwd(i + "/");
         }
      } else {
         if(words[1] == "/"){
            return;
         }
         state.set_cwd(initial_cwd);
         state.set_pwd(initial_pwd);
         throw command_error("cd: " + words[1] 
            + ": No such directory");
      }
   }
}

void fn_echo (inode_state& state, const wordvec& words) {
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   cout << word_range (words.cbegin() + 1, words.cend()) << endl;
}

void fn_exit (inode_state& state, const wordvec& words) {
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   throw ysh_exit();
}

void fn_ls (inode_state& state, const wordvec& words) {
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   DEBUGS ('l', 
      const auto& dirents = state.get_root()->get_dirents();
      for (const auto& entry: dirents) {
         cerr << "\"" << entry.first << "\"->" << entry.second << endl;
      }
    );

   inode_ptr initial_cwd = state.get_cwd();
   string directory = 
      (state.get_cwd() == state.get_root()) ? "/" : ".";
   if(words.size() != 1){
      directory = words[1];
      vector<string> paths = splitPath(words[1]);
      if(words[1][0] == '/'){
         state.set_cwd(state.get_root());
      }
      if(words[1] == "/"){
         paths = {"."};
      }

      inode_ptr newDir;
      for(int i=0; i < static_cast<int>(paths.size()); i++){
         directory_entries dirents = state.get_cwd()->get_dirents();
         if(dirents.find(paths[i]) != dirents.end()){
            state.set_cwd(dirents.at(paths[i]));
         } else {
            state.set_cwd(initial_cwd);
            throw command_error("ls: " + words[1] 
               + ": No such file or directory");
         }
      }
   }
   cout << directory << ": " << endl;
   for(auto i: state.get_cwd()->get_dirents()){
      cout << setw(8) << i.second->get_inode_nr() << "      "
         << i.second->getFilePtr()->size() << "  " << i.first;
      if(i.second->getFilePtr()->isDirectory() == "directory") 
         cout << "/";
      cout << endl;
   }

   state.set_cwd(initial_cwd);
}


void fn_lsr (inode_state& state, const wordvec& words) {
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   inode_ptr initial_cwd = state.get_cwd();
   string directory = 
      (state.get_cwd() == state.get_root()) ? "/" : ".";
   if(words.size() != 1){
      directory = words[1];
      vector<string> paths = splitPath(words[1]);
      if(words[1][0] == '/'){
         state.set_cwd(state.get_root());
      }
      if(words[1] == "/"){
         paths = {"."};
      }
      
      inode_ptr newDir;
      for(int i=0; i < static_cast<int>(paths.size()); i++){
         directory_entries dirents = state.get_cwd()->get_dirents();
         if(dirents.find(paths[i]) != dirents.end()){
            state.set_cwd(dirents.at(paths[i]));
         } else {
            state.set_cwd(initial_cwd);
            throw command_error("lsr: " +
                words[1] + ": Directory does not exist");
         }
      }
   }
   vector<string> toPrint;
   cout << directory << ((directory == "/") ? ": " : "/:")<< endl;
   for(auto i: state.get_cwd()->get_dirents()){
      cout << setw(8) << i.second->get_inode_nr() << "      "
         << i.second->getFilePtr()->size() << "  " << i.first;
      if(i.second->getFilePtr()->isDirectory() == "directory"){
         if(i.first != "." and i.first != "..") 
            toPrint.push_back(i.first);
         cout << "/";
      }
      cout << endl;
   }
   for(auto j: toPrint){
      wordvec tempwords;
      tempwords.push_back("lsr");
      string temp;
      if(words.size() == 1){
         if(state.get_cwd() == state.get_root()) temp = "/" + j;
         else temp = j;
      } else temp = (words[1] == "/") ?
         words[1] + j : words[1] + "/" + j;
      tempwords.push_back(temp);
      state.set_cwd(initial_cwd);
      fn_lsr(state, tempwords);
   }
   state.set_cwd(initial_cwd);
}


void fn_make (inode_state& state, const wordvec& words) {
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   if(words.size() <= 1)
      throw command_error("make: Not enough arguments");

   inode_ptr initial_cwd = state.get_cwd();
   if(words[1][0] == '/'){
      state.set_cwd(state.get_root());
   }

   vector<string> paths = splitPath(words[1]);
   inode_ptr newFile;
   for(int i=0; i < static_cast<int>(paths.size()); i++){
      directory_entries dirents = state.get_cwd()->get_dirents();
      if(dirents.find(paths[i]) == dirents.end()){
         if(i == static_cast<int>(paths.size()) - 1){
            newFile = dirents.at(".")->getFilePtr()->mkfile(paths[i]);
            wordvec data(words.begin() + 2, words.end());
            newFile->getFilePtr()->writefile(data);
            state.set_cwd(initial_cwd);
            return;
         }
         throw command_error("make: Directory does not exist");
         return;
      } else {
         if(i == static_cast<int>(paths.size()) - 1 and
             dirents.at(paths[i])->getFilePtr()->isDirectory() !=
               "directory"){
            newFile = dirents.at(paths[i]);
            wordvec data(words.begin() + 2, words.end());
            newFile->getFilePtr()->writefile(data);
            state.set_cwd(initial_cwd);
            return;
         } else if(dirents.at(paths[i])->getFilePtr()
            ->isDirectory() == "directory") {
            throw command_error("make: " + paths[i] +
               " is a directory");
         }
         state.set_cwd(dirents.at(paths[i]));
         continue;
      }
   }
   state.set_cwd(initial_cwd);
}

void fn_mkdir (inode_state& state, const wordvec& words) {
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   if(words.size() <= 1)
      throw command_error("mkdir: Not enough arguments");

   inode_ptr initial_cwd = state.get_cwd();
   if(words[1][0] == '/'){
      state.set_cwd(state.get_root());
   }

   vector<string> paths = splitPath(words[1]);
   inode_ptr newDir;
   for(int i=0; i < static_cast<int>(paths.size()); i++){
      directory_entries dirents = state.get_cwd()->get_dirents();
      if(dirents.find(paths[i]) == dirents.end()){
         if(i != static_cast<int>(paths.size())-1){
            state.set_cwd(initial_cwd);
            throw command_error("mkdir: " + paths[i] 
               + ": No such file or directory");
         }
         newDir = dirents.at(".")->getFilePtr()->mkdir(paths[i]);
         directory_entries& newdirents = newDir->get_dirents();
         newdirents.insert({".",newDir});
         newdirents.insert({"..",state.get_cwd()});
         state.set_cwd(newDir);
      } else {
         if(i == static_cast<int>(paths.size()) - 1){
            throw command_error("mkdir: " + paths[i] 
               + ": Directory already exists");
         }
         state.set_cwd(dirents.at(paths[i]));
         continue;
      }
   }
   state.set_cwd(initial_cwd);
}

void fn_prompt (inode_state& state, const wordvec& words) {
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   if(words.size() <= 1)
      throw command_error("prompt: Not enough arguments");
   string temp;
   for(int i=1; i < static_cast<int>(words.size()); i++)
      temp = temp + words[i] + " ";
   state.prompt(temp);
}

void fn_pwd (inode_state& state, const wordvec& words) {
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   if(words.size() != 1) throw command_error("pwd: Too many arguments");
   for(int i=0; i < static_cast<int>(state.get_pwd().size()); i++){
      if(i == static_cast<int>(state.get_pwd().size()) -1 and 
         static_cast<int>(state.get_pwd().size()) > 1){
         cout << state.get_pwd()[i]
            .substr(0, state.get_pwd()[i].size()-1);
      } else{
         cout << state.get_pwd()[i];
      }
      
   }
   cout << endl;
}

void fn_rm (inode_state& state, const wordvec& words) {
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   if(words.size() > 2) throw command_error("rm: too many arguments");
   inode_ptr initial_cwd = state.get_cwd();
   if(words.size() != 1){
      if(words[1][0] == '/'){
         state.set_cwd(state.get_root());
      }

      vector<string> paths = splitPath(words[1]);
      inode_ptr newDir;
      for(int i=0; i < static_cast<int>(paths.size()); i++){
         directory_entries dirents = state.get_cwd()->get_dirents();
         if(dirents.find(paths[i]) != dirents.end()){
            if(i == static_cast<int>(paths.size()) - 1){
               inode_ptr toDelete = dirents.at(paths[i]);
               if(toDelete->getFilePtr()->isDirectory() !=
                   "directory"){
                  dirents.at(".")->getFilePtr()->remove(paths[i]);
               } else {
                  if(toDelete->get_dirents().size() > 2){
                     state.set_cwd(initial_cwd);
                     throw command_error(
                        "Directory is not empty, cannot delete");
                  }
                  dirents.at(".")->getFilePtr()->remove(paths[i]);
               }
            }
            state.set_cwd(dirents.at(paths[i]));
         } else {
            state.set_cwd(initial_cwd);
            throw command_error("rm: " + words[1] +
                ": No such file or directory");
         }
      }
   }
   state.set_cwd(initial_cwd);
}

void fn_rmr (inode_state& state, const wordvec& words) {
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   inode_ptr initial_cwd = state.get_cwd();
   inode_ptr prev_cwd = state.get_cwd();
   if(words.size() != 1){

      if(words[1][0] == '/'){
         state.set_cwd(state.get_root());
      }
      if(words[1] == "/") 
         throw command_error("rmr: Cannot delete root directory");
      if(words[1] == "." or words[1] == "..")
         throw command_error("rmr: Cannot delete this directory");

      vector<string> paths = splitPath(words[1]);
      vector<string> toRemove;
      inode_ptr newDir;
      for(int i=0; i < static_cast<int>(paths.size()); i++){
         directory_entries dirents = state.get_cwd()->get_dirents();
         if(dirents.find(paths[i]) != dirents.end()){
            if(i == static_cast<int>(paths.size()) - 1){
               state.clearDir(dirents.at(paths[i]));
               dirents.at(".")->getFilePtr()->remove(paths[i]);
               state.set_cwd(initial_cwd);
               return;
            }
            prev_cwd = state.get_cwd();
            state.set_cwd(dirents.at(paths[i]));
         } else {
            if(i == static_cast<int>(paths.size()) - 1){
               state.set_cwd(initial_cwd);
               throw command_error("rmr: " + words[1] 
                  + ": No such file or directory");
            }
            state.set_cwd(initial_cwd);
            throw command_error("rmr: " + words[1] 
               + ": No such directory");
         }
      }
      state.set_cwd(initial_cwd);
   } else {
      throw command_error("rmr: Not enough arguments");
   }  
}
