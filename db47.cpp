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

enum class DbError {
  SUCCESS = 0,
  FILE_OPEN_FAILED,
  FILE_LOCK_FAIL,
  FSYNC_FAIL,
  RENAME_FAIL,
  KEY_NOT_FOUND,
};

void logError(const std::string &message) { std::cerr << message << endl; }

bool lockDatabase(int fd) {
  if (flock(fd, LOCK_EX) == -1) {
    logError("Failed to acquire file lock");
    return false;
  }
  return true;
}

bool unlockDatabase(int fd) {
  if (flock(fd, LOCK_UN) == -1) {
    logError("Failed to release file lock");
    return false;
  }
  return true;
}

DbError db_get(const std::string &key, std::string &value) {
  std::ifstream database(database_name);
  if (!database.is_open()) {
    logError("Failed to open database");
    return DbError::FILE_OPEN_FAILED;
  }

  std::string line;
  while (std::getline(database, line)) {
    std::stringstream current_line(line);
    std::string key_stored;
    if (getline(current_line, key_stored, ',') && key_stored == key) {
      getline(current_line, value);
      database.close();
      return DbError::SUCCESS;
    }
  }
  database.close();
  return DbError::KEY_NOT_FOUND;
}

DbError db_set(const std::string &key, const std::string &value) {
  int fd_db = open(database_temp_name.c_str(), O_WRONLY | O_CREAT, 0644);
  if (fd_db == -1) {
    logError("Failed to open temporary database file");
    return DbError::FILE_OPEN_FAILED;
  }

  if (!lockDatabase(fd_db)) {
    close(fd_db);
    return DbError::FILE_LOCK_FAIL;
  }

  std::ifstream database(database_name);
  std::ofstream database_temp(database_temp_name);
  if (!database.is_open() || !database_temp.is_open()) {
    logError("Failed to open databases");
    unlockDatabase(fd_db);
    close(fd_db);
    return DbError::FILE_OPEN_FAILED;
  }

  std::string line;
  while (std::getline(database, line)) {
    database_temp << line << endl;
  }
  database.close();
  database_temp << key << "," << value << endl;
  database_temp.close();

  if (fsync(fd_db) == -1) {
    logError("fsync failed");
    unlockDatabase(fd_db);
    close(fd_db);
    return DbError::FSYNC_FAIL;
  }

  if (rename(database_temp_name.c_str(), database_name.c_str()) == -1) {
    logError("Failed to rename temporary database to main database");
    unlockDatabase(fd_db);
    close(fd_db);
    return DbError::RENAME_FAIL;
  }

  if (!unlockDatabase(fd_db)) {
    close(fd_db);
    return DbError::FILE_LOCK_FAIL;
  }

  close(fd_db);
  return DbError::SUCCESS;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    logError("USAGE: " + std::string(argv[0]) + " {get|set} key [value]");
    return EXIT_FAILURE;
  }

  const std::string operation = argv[1];
  const std::string key = argv[2];

  if (operation == "set") {
    if (argc < 4) {
      logError("USAGE: " + std::string(argv[0]) + " set key value");
      return EXIT_FAILURE;
    }
    const std::string value = argv[3];
    DbError set_status = db_set(key, value);
    if (set_status != DbError::SUCCESS) {
      logError("Failed to set key-value in the database");
      return EXIT_FAILURE;
    }
    std::cout << "New record has been inserted in the database." << endl;
  } else if (operation == "get") {
    std::string value;
    DbError get_status = db_get(key, value);
    if (get_status != DbError::SUCCESS) {
      logError("Failed to get key from the database");
      return EXIT_FAILURE;
    }
    if (!value.empty()) {
      std::cout << value << endl;
    } else {
      std::cout << "Key is not found in the database" << endl;
    }
  } else {
    logError("USAGE: " + std::string(argv[0]) + " {get|set} key [value]");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
