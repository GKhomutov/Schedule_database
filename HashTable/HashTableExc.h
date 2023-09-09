#ifndef HASH_TABLE_EXC_H
#define HASH_TABLE_EXC_H

#include <exception>

class HashTableExc : public std::exception {
	const char *msg;
  public:
	HashTableExc(const char *msg) : msg(msg) {}
	virtual const char *what() const noexcept override { return msg; }
};

class HashTableExcMem : public HashTableExc {
  public:
	HashTableExcMem(const char *msg) : HashTableExc(msg) {}
};

class HashTableExcKey : public HashTableExc {
  public:
	HashTableExcKey(const char *msg) : HashTableExc(msg) {}
};

#endif // HASH_TABLE_EXC_H
