#ifndef FACTORY_EXC_H
#define FACTORY_EXC_H

#include <exception>

class FactoryExc : public std::exception {
	const char *msg;
  public:
	FactoryExc(const char *msg) : msg(msg) {}
	virtual const char *what() const noexcept override { return msg; }
};

class FactoryExcIdConflict : public FactoryExc {
public:
	FactoryExcIdConflict(const char *msg) : FactoryExc(msg) {}
};

class FactoryExcNoId : public FactoryExc {
public:
	FactoryExcNoId(const char *msg) : FactoryExc(msg) {}
};

#endif // FACTORY_EXC_H
