#ifndef QUERY_H
#define QUERY_H

#include <istream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>
#include <cctype>
#include <variant>
#include <utility>
#include "QueryExc.h"
#include "../Factory/factory.hpp"
#include "../TaskStructures/task_structures.h"

/* Виды запросов. */
typedef enum { VOID, STOP, SHUTDOWN, INSERT, REMOVE, SELECT, RESELECT, PRINT } QueryType;

class Query
{
  private:
	static const Factory<Query, std::string>& factory();

  protected:
	Field recognize_field(std::string name) const;
	
  public:
	static Query* create_query(const std::string &str);
	virtual ~Query() {}
	virtual void parse(std::istream &is) = 0;
	virtual QueryType type() const { return VOID; }
};

class StopQuery : public Query
{
  public:
	virtual void parse(std::istream &is) override;
	virtual QueryType type() const override { return STOP; }
};

class ShutdownQuery : public Query
{
  public:
	virtual void parse(std::istream &is) override;
	virtual QueryType type() const override { return SHUTDOWN; }
};

class ConditionalQuery : public Query
{
  private:
	typedef enum {LEFT, RIGHT, SINGLE} BoundaryType;
	int recognize_int(Field field, const std::string &text, BoundaryType bt) const;

  protected:
	std::vector<Condition> _conditions;
	static bool cmp(const Condition &c1, const Condition &c2) { return c1.field < c2.field; }
	Condition merge_conditions(const Condition &c1, const Condition &c2) const;

  public:
	ConditionalQuery() {}
	ConditionalQuery(const std::vector<Condition> &conditions);
	virtual void parse(std::istream &is) override;
	const std::vector<Condition>& conditions() const { return _conditions; }
	ConditionalQuery& operator*=(const ConditionalQuery &other);
};

class InsertQuery : public ConditionalQuery
{
  public:
	InsertQuery() {}
	InsertQuery(const Record &record);
	virtual void parse(std::istream &is) override;
	virtual QueryType type() const override { return INSERT; }
};

class RemoveQuery : public ConditionalQuery
{
  public:
	virtual QueryType type() const override { return REMOVE; }
};

class SelectQuery : public ConditionalQuery
{
  public:
	virtual QueryType type() const override { return SELECT; }
};

class ReselectQuery : public ConditionalQuery
{
  public:
	virtual QueryType type() const override { return RESELECT; }
};

class PrintQuery : public Query
{
  private:
	std::vector<Field> _fields;
	std::vector<Field> _sortby;

  public:
	virtual void parse(std::istream &is) override;
	virtual QueryType type() const override { return PRINT; }
	const std::vector<Field>& fields() const { return _fields; }
	const std::vector<Field>& sortby() const { return _sortby; }
};



class QueryResult
{
  private:
	// Сообщение об ошибке или строки таблицы, запрошенные командой print
	using InfoForClient = std::variant< const char*, std::vector<std::string> >;

	ServerCode _servcode;	// Информация для сервера (например, отключить клиента)
	ProtocolCode _protcode;	// Код результата в соответствии с протоколом взаимодействия сервер-клиент
	InfoForClient _info;

	static void send_int(int fd, int number);
	static void send_str(int fd, const char *str);

  public:
	QueryResult() {}
	void set_servcode(ServerCode code) { _servcode = code; }
	void set_protcode(ProtocolCode code) { _protcode = code; }
	void set_info(const InfoForClient &info) { _info = info; }
	ServerCode get_servcode() const { return _servcode; }
	void send_result(int fd) const;
};


#endif // QUERY_H
