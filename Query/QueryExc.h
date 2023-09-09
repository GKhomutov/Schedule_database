#ifndef QUERY_EXC_H
#define QUERY_EXC_H

#include <exception>

class QueryExc : public std::exception {
	const char *msg;
  public:
	QueryExc(const char *msg) : msg(msg) {}
	virtual const char *what() const noexcept override { return msg; }
};

class QueryExcSyntax : public QueryExc {
  public:
	QueryExcSyntax(const char *msg) : QueryExc(msg) {}
};

class QueryExcValue : public QueryExc {
  public:
	QueryExcValue(const char *msg) : QueryExc(msg) {}
};

class QueryExcSend : public QueryExc {
  public:
	QueryExcSend(const char *msg) : QueryExc(msg) {}
};

#endif // QUERY_EXC_H
