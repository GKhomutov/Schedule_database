#ifndef GENERATOR_EXC_H
#define GENERATOR_EXC_H

#include <exception>

class GeneratorExc : public std::exception
{
	const char *msg;

public:
	GeneratorExc(const char *msg) : msg(msg) {}
	virtual const char *what() const noexcept override { return msg; }
};

class GeneratorExcFile : public GeneratorExc
{
public:
	GeneratorExcFile(const char *msg) : GeneratorExc(msg) {}
};

#endif // GENERATOR_EXC_H
