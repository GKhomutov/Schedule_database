#ifndef DATABASE_EXC_H
#define DATABASE_EXC_H

#include <exception>

class DatabaseExc : public std::exception {
	const char *msg;
  public:
	DatabaseExc(const char *msg) : msg(msg) {}
	virtual const char *what() const noexcept override { return msg; }
};

class DatabaseExcFile : public DatabaseExc {
  public:
	DatabaseExcFile(const char *msg) : DatabaseExc(msg) {}
};

class DatabaseExcUser : public DatabaseExc {
  public:
	DatabaseExcUser(const char *msg) : DatabaseExc(msg) {}
};

#endif // DATABASE_EXC_H
