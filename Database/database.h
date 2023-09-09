#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <fstream>
#include <cassert>
#include "DatabaseExc.h"
#include "../Query/query.h"
#include "../HashTable/HashTable.hpp"
#include "../TaskStructures/task_structures.h"

class Database
{
  private:
	using NameSchedule = HashTable< std::string, std::vector<SchedulePosition> >;
	
	ScheduleItem _schedule[NUM_OF_PERIODS * NUM_OF_DAYS][NUM_OF_ROOMS + 1];
	NameSchedule _teachers;
	NameSchedule _subjects;

	using UserId = int;	// не хочу шаблон делать, некрасиво
	struct Session {
	  SelectQuery select_query;
	  QueryType last_query;
	};
	std::map<UserId, Session> _sessions;

	static void name_remove(NameSchedule &ns, const std::string &name, const SchedulePosition &pos);
	Record get_record(const SchedulePosition &pos) const;
	bool match(const SchedulePosition &pos, const Condition &cond) const;
	bool match(const SchedulePosition &pos, const ConditionalQuery &query) const;
	std::vector<SchedulePosition> find(const ConditionalQuery &query) const;

	QueryResult insert(const UserId &user, const Query *query);
	QueryResult remove(const UserId &user, const Query *query);
	QueryResult select(const UserId &user, const Query *query);
	QueryResult reselect(const UserId &user, const Query *query);
	QueryResult print(const UserId &user, const Query *query);
	QueryResult shutdown(const UserId &user, const Query *query = nullptr);

	using QueryExecutor = QueryResult (Database::*)(const UserId&, const Query *);
	using Scripts = std::map<QueryType, QueryExecutor>;
	static const Scripts& scripts();

  public:
	Database() {}
	void from_file(const std::string &filename);
	void to_file(const std::string &filename) const;
	QueryResult process_query(const UserId &user, const std::string &str);
	bool add_user(const UserId &user);
	QueryResult remove_user(const UserId &user, const Query *query = nullptr);
};

#endif // DATABASE_H
