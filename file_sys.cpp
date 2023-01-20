// $Id: file_sys.cpp,v 1.13 2022-01-26 16:10:48-08 - - $

#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace std;

#include "debug.h"
#include "file_sys.h"

size_t inode::next_inode_nr {1};

ostream& operator<< (ostream& out, file_type type) {
   switch (type) {
      case file_type::PLAIN_TYPE: out << "PLAIN_TYPE"; break;
      case file_type::DIRECTORY_TYPE: out << "DIRECTORY_TYPE"; break;
      default: assert (false);
   };
   return out;
}

inode_state::inode_state() {
   root = cwd = make_shared<inode> (file_type::DIRECTORY_TYPE);
   directory_entries& dirents = root->get_dirents();
   dirents.insert (dirent_type (".", root));
   dirents.insert (dirent_type ("..", root));
   DEBUGF ('i', "root = " << root << ", cwd = " << cwd
           << ", prompt = \"" << prompt() << "\""
           << ", file_type = " << root->contents->file_type());
}

inode_state::~inode_state(){
   clearDir(root);
}

void inode_state::clearDir(inode_ptr dir){
   directory_entries& dirents = dir->get_dirents();
   for(auto i: dirents){
      if(i.second->getFilePtr()->isDirectory() == "directory" and 
         i.first != "." and i.first != ".."){
         if(i.second->getFilePtr()->get_dirents().size() > 2)
            clearDir(i.second);
      }
   }
   dirents.clear();
}

const string& inode_state::prompt() const { return prompt_; }

void inode_state::prompt (const string& new_prompt) {
   prompt_ = new_prompt;
}

void inode_state::set_cwd(inode_ptr ptr){
   cwd = ptr;
}

void inode_state::add_pwd(string str){
   pwd.push_back(str);
}

void inode_state::remove_pwd(){
   pwd.pop_back();
}

void inode_state::reset_pwd(){
   pwd = {"/"};
}

void inode_state::set_pwd(vector<string> newpwd){
   pwd = newpwd;
}

ostream& operator<< (ostream& out, const inode_state& state) {
   out << "inode_state: root = " << state.root
       << ", cwd = " << state.cwd;
   return out;
}

inode::inode(file_type type): inode_nr (next_inode_nr++) {
   switch (type) {
      case file_type::PLAIN_TYPE:
           contents = make_shared<plain_file>();
           break;
      case file_type::DIRECTORY_TYPE:
           contents = make_shared<directory>();
           break;
      default: assert (false);
   }
   DEBUGF ('i', "inode " << inode_nr << ", type = " << type);
}

size_t inode::get_inode_nr() const {
   DEBUGF ('i', "inode = " << inode_nr);
   return inode_nr;
}

directory_entries& inode::get_dirents() {
   return contents->get_dirents();
}


file_error::file_error (const string& what):
            runtime_error (what) {
}

const wordvec& base_file::readfile() const {
   cout << "HERE" << endl;
   throw file_error ("is a " + file_type());
}

void base_file::writefile (const wordvec&) {
   throw file_error ("is a " + file_type());
}

void base_file::remove (const string&) {
   throw file_error ("is a " + file_type());
}

inode_ptr base_file::mkdir (const string&) {
   throw file_error ("is a " + file_type());
}

inode_ptr base_file::mkfile (const string&) {
   throw file_error ("is a " + file_type());
}

directory_entries& base_file::get_dirents() {
   throw file_error ("is a " + file_type());
}


size_t plain_file::size() const {
   size_t size {0};
   DEBUGF ('i', "size = " << size);
   if(data.size() == 0){
      return size;
   }
   for(auto i: data){
      size += i.size();
   }
   size += data.size() - 1;
   return size;
}

const wordvec& plain_file::readfile() const {
   DEBUGF ('i', data);
   return data;
}

void plain_file::writefile (const wordvec& words) {
   DEBUGF ('i', words);
   data.clear();
   for(auto i: words){
      data.push_back(i);
   }
}

size_t directory::size() const {
   size_t size {0};
   DEBUGF ('i', "size = " << size);
   size = dirents.size();
   return size;
}

void directory::remove (const string& filename) {
   DEBUGF ('i', filename);
   // cout << "--__--__--_--" << endl;
   // for(auto j: dirents) cout << j.first << endl;
   // cout << endl;
   auto toRemove = dirents.find(filename);
   if(toRemove == dirents.end()){
      cout << "file or dir " << filename << " does not exist" << endl;
      return;
   }
   dirents.erase(toRemove);
}

inode_ptr directory::mkdir (const string& dirname) {
   DEBUGF ('i', dirname);
   inode_ptr newDir(new inode(file_type::DIRECTORY_TYPE));
   get_dirents().insert({dirname, newDir});
   return newDir;
}

inode_ptr directory::mkfile (const string& filename) {
   DEBUGF ('i', filename);
   inode_ptr newFile(new inode(file_type::PLAIN_TYPE));
   get_dirents().insert({filename, newFile});
   return newFile;
}

directory_entries& directory::get_dirents() {
   return dirents;
}

