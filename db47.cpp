/*
 * define two functions : get and set (done)
 * - get : to get a record from the database. (done)
 * - set : to set a new record to the database. (done)
 * read arg form command line ./exec {get|set} key [value] (done)
 * use <fstream> to create, write to, read from files (done)

 * implement ACID : Atomicity - Consistency - Isolation - Durability
 * Durability : can be afforded using fsync to ensure data is written directly
 * to the disk Isolation : to have multiprocess isolation, we use flock(db_fd,
 * LOCK_EX) Rename for Atomicity, in case the machine crashes while we calling
 * db_set, we need another file as a temp and we we append to the main database
 * if db_set is done (we use rename in this case)
 * when we want to use system calls(close, fsync, flock...) we need open from
 * fcntl and unistd otherwise for Read-Write ifstream/ofstream would be the way
 * to go!
 *
 */

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/file.h>
#include <unistd.h>

#define endl "\n"
const std::string database_name = "database";
const std::string database_temp_name = "database_temp";

int db_get(const std::string &key, std::string &value) {
  std::ifstream database(database_name);
  std::string line;
  if (!database.is_open()) {
    std::cerr << "Database could not be opened" << endl;
    return EXIT_FAILURE;
  }
  while (std::getline(database, line)) {
    std::stringstream ss(line);
    std::string stored_key;
    if (std::getline(ss, stored_key, ',') && stored_key == key) {
      std::getline(ss, value);
      return EXIT_SUCCESS;
    }
  }
  database.close();

  return EXIT_FAILURE;
}

int db_set(const std::string &key, const std::string &value) {
  // 0644 represents the permissions (TBS)
  int temp_fd = open(database_temp_name.c_str(), O_WRONLY | O_CREAT, 0644);
  if (temp_fd == -1) {
    std::cerr << "Failed to open temporary database for open/create" << endl;
    close(temp_fd);
    return EXIT_FAILURE;
  }

  int db_fd = open(database_name.c_str(), O_RDWR);
  if (db_fd == -1) {
    std::cerr << "Failed to open database file for locking" << endl;
    close(db_fd);
    return EXIT_FAILURE;
  }

  // we need to lock temp_fd
  if (flock(db_fd, LOCK_EX) == -1) {
    std::cerr << "Failed to lock the file" << endl;
    close(db_fd);
    close(temp_fd);
    return EXIT_FAILURE;
  }

  // we need to copy content from database to temp database
  std::ifstream database_stream(database_name);
  std::ofstream temp_database_stream(database_temp_name);
  if (!database_stream.is_open() || !temp_database_stream.is_open()) {
    std::cerr << "Failed to open database files" << endl;
    close(db_fd);
    close(temp_fd);
    return EXIT_FAILURE;
  }

  std::string line;
  while (std::getline(database_stream, line)) {
    temp_database_stream << line << "\n";
  }

  database_stream.close();

  // add new record passed as args
  temp_database_stream << key << "," << value << endl;
  temp_database_stream.close();

  if (fsync(temp_fd) == -1) {
    std::cerr << "fsync failed on temp" << endl;
    close(db_fd);
    close(temp_fd);
    return EXIT_FAILURE;
  }
  close(temp_fd);

  if (rename(database_temp_name.c_str(), database_name.c_str()) == -1) {
    std::cerr << "Failed to rename database file" << endl;
    close(db_fd);
    return EXIT_FAILURE;
  }

  if (flock(db_fd, LOCK_UN) == -1) {
    std::cerr << "Failed to release lock on database file" << endl;
    close(db_fd);
    return EXIT_FAILURE;
  }

  close(db_fd);
  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "USAGE : " << argv[0] << " {get|set} key [value]" << endl;
    return EXIT_FAILURE;
  }

  // op is whether set or get
  const std::string operation = *(argv + 1);
  const std::string key = *(argv + 2);

  if (operation == "set") {
    if (argc < 4) {
      std::cerr << "USAGE : " << argv[0] << " set key value" << endl;
      return EXIT_FAILURE;
    }
    // now we have the key and value, we need to store them in the database
    const std::string value = *(argv + 3);
    const int set_status = db_set(key, value);
    // if db_set mess up!
    if (set_status == 1) {
      std::cerr << "DB_SET is not working as expected" << endl;
      return EXIT_FAILURE;
    }
    std::cout << "New record has been inserted in the database." << endl;
  } else if (operation == "get") {
    std::string value;
    const int get_status = db_get(key, value);
    if (get_status == 1) {
      std::cerr << "DB_GET is not working as expected" << endl;
      return EXIT_FAILURE;
    }
    if (!value.empty()) {
      std::cout << value << endl;
    } else {
      std::cout << "Key is not found in the database" << endl;
    }
  } else {
    std::cerr << "USAGE : " << argv[0] << " set key value" << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
