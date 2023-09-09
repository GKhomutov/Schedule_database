#include "database.h"


/* -----------------------------------------PRIVATE METHODS-------------------------------------- */


void Database::name_remove(NameSchedule &ns, const std::string &name, const SchedulePosition &pos)
{
	auto it = ns.find(name);
	auto &positions = it.val();
	int n = positions.size();
	for (int i = 0; i < n-1; ++i) {
		if (positions[i] == pos) {
			positions[i] = positions[n - 1];
			break;
		}
	}
	positions.pop_back();
	if (positions.empty())
		ns.erase(name);
}


Record Database::get_record(const SchedulePosition &pos) const
{
	Record record;
	const ScheduleItem &item = _schedule[pos.timecode][pos.room];
	record.teacher = item.teacher;
	record.subject = item.subject;
	record.room = pos.room;
	record.time = pos.timecode;
	record.group = item.group;
	return record;
}


bool Database::match(const SchedulePosition &pos, const Condition &cond) const
{
	const ScheduleItem &item = _schedule[pos.timecode][pos.room];
	if (cond.field == TEACHER || cond.field == SUBJECT) {
		const auto &name = std::get<std::string>(cond.value);
		const std::string &candidate = (cond.field == TEACHER ? item.teacher : item.subject);
		if (cond.relation == EQUAL)
			return candidate == name;
		return candidate.starts_with(name);
	}

	int candidate;
	if (cond.field == ROOM)
		candidate = pos.room;
	else if (cond.field == DAY)
		candidate = Time(pos.timecode).day;
	else if (cond.field == PERIOD)
		candidate = Time(pos.timecode).period;
	else if (cond.field == GROUP)
		candidate = item.group;
	
	if (cond.relation == EQUAL)
		return candidate == std::get<int>(cond.value);
	const auto &range = std::get<std::pair<int, int>>(cond.value);
	return candidate >= range.first && candidate <= range.second;
}


bool Database::match(const SchedulePosition &pos, const ConditionalQuery &query) const
{
	if (_schedule[pos.timecode][pos.room].empty())
		return false;
	for (const auto &cond : query.conditions())
		if (!match(pos, cond))
			return false;
	return true;
}


std::vector<SchedulePosition> Database::find(const ConditionalQuery &query) const
{
	std::vector<SchedulePosition> ans;
	std::pair<int, int> room = {0, NUM_OF_ROOMS}, day = {1, NUM_OF_DAYS}, period = {1, NUM_OF_PERIODS};
	for (const auto &cond : query.conditions())
	{
		if (cond.field == TEACHER && cond.relation == EQUAL) {
			const auto &teacher = std::get<std::string>(cond.value);
			if (_teachers.find(teacher) != _teachers.cend())
				for (const SchedulePosition &pos : _teachers[teacher])
					if (match(pos, query))
						ans.push_back(pos);
			return ans;
		}
		if (cond.field == SUBJECT && cond.relation == EQUAL) {
			const auto &subject = std::get<std::string>(cond.value);
			if (_subjects.find(subject) != _subjects.cend())
				for (const SchedulePosition &pos : _subjects[subject])
					if (match(pos, query))
						ans.push_back(pos);
			return ans;
		}
		
		if (cond.field == ROOM) {
			room = cond.get_range();
		} else if (cond.field == DAY) {
			day = cond.get_range();
		} else if (cond.field == PERIOD) {
			period = cond.get_range();
		}
	}
	for (int d = day.first; d <= day.second; ++d) {
		for (int p = period.first; p <= period.second; ++p) {
			for (int r = room.first; r <= room.second; ++r) {
				SchedulePosition pos({d, p}, r);
				if (match(pos, query))
					ans.push_back(pos);
			}
		}
	}
	return ans;
}


QueryResult Database::insert(const UserId &user, const Query *query)
{
	auto q = dynamic_cast<const InsertQuery*>(query);
	assert(q != nullptr && "Bad cast in insert");

	QueryResult result;
	result.set_servcode(SEND_INFO);
	const auto &conds = q->conditions();

	std::vector<Condition> conflict = {conds[DAY], conds[PERIOD], conds[ROOM]};
	if (!find(conflict).empty()) {
		result.set_protcode(ERROR);
		result.set_info("The room is occupied at this time!");
		return result;
	}
	conflict.back() = conds[TEACHER];
	if (!find(conflict).empty()) {
		result.set_protcode(ERROR);
		result.set_info("The teacher is busy at this time!");
		return result;
	}
	conflict.back() = conds[GROUP];
	if (!find(conflict).empty()) {
		result.set_protcode(ERROR);
		result.set_info("The group is busy at this time!");
		return result;
	}

	Time time(std::get<int>(conds[DAY].value), std::get<int>(conds[PERIOD].value));
	int room = std::get<int>(conds[ROOM].value);
	const auto& teacher = std::get<std::string>(conds[TEACHER].value);
	const auto& subject = std::get<std::string>(conds[SUBJECT].value);
	_schedule[time][room].used = true;
	_schedule[time][room].group = std::get<int>(conds[GROUP].value);
	_schedule[time][room].teacher = teacher;
	_schedule[time][room].subject = subject;
	_teachers[teacher].push_back({time, room});
	_subjects[subject].push_back({time, room});

	_sessions[user].last_query = INSERT;
	result.set_protcode(SUCCESS);
	return result;
}


QueryResult Database::remove(const UserId &user, const Query *query)
{
	auto q = dynamic_cast<const RemoveQuery*>(query);
	assert(q != nullptr && "Bad cast in remove");
	QueryResult result;
	std::vector<SchedulePosition> to_remove = find(*q);
	for (const auto &pos : to_remove) {
		ScheduleItem &item = _schedule[pos.timecode][pos.room];
		name_remove(_teachers, item.teacher, pos);
		name_remove(_subjects, item.subject, pos);
		item.clear();
	}
	_sessions[user].last_query = REMOVE;
	result.set_protcode(SUCCESS);
	result.set_servcode(SEND_INFO);
	return result;
}


QueryResult Database::select(const UserId &user, const Query *query)
{
	auto q = dynamic_cast<const SelectQuery *>(query);
	assert(q != nullptr && "Bad cast in select");
	QueryResult result;
	_sessions[user].select_query = *q;
	_sessions[user].last_query = SELECT;
	result.set_protcode(SUCCESS);
	result.set_servcode(SEND_INFO);
	return result;
}


QueryResult Database::reselect(const UserId &user, const Query *query)
{
	auto q = dynamic_cast<const ReselectQuery *>(query);
	assert(q != nullptr && "Bad cast in reselect");
	
	QueryResult result;
	result.set_servcode(SEND_INFO);

	QueryType last = _sessions[user].last_query;
	if (last != SELECT && last != RESELECT && last != PRINT) {
		result.set_protcode(ERROR);
		result.set_info("Your last query should be \"select\", \"reselect\" or \"print\"!");
		return result;
	}

	_sessions[user].select_query *= (*q);
	_sessions[user].last_query = RESELECT;
	result.set_protcode(SUCCESS);
	return result;
}


QueryResult Database::print(const UserId &user, const Query *query)
{
	auto q = dynamic_cast<const PrintQuery *>(query);
	assert(q != nullptr && "Bad cast in print");

	QueryResult result;
	result.set_servcode(SEND_INFO);

	QueryType last = _sessions[user].last_query;
	if (last != SELECT && last != RESELECT && last != PRINT) {
		result.set_protcode(ERROR);
		result.set_info("Your last query should be \"select\", \"reselect\" or \"print\"!");
		return result;
	}
	
	const SelectQuery &select_query = (_sessions.find(user)->second).select_query;
	std::vector<SchedulePosition> positions = find(select_query);
	std::vector<Record> records;
	for (const auto &pos : positions)
		records.push_back(get_record(pos));

	std::sort(records.begin(), records.end(), [q](const Record &r1, const Record &r2) {
		for (Field field : q->sortby()) {
			if (field == TEACHER) {
				if (r1.teacher != r2.teacher)
					return r1.teacher < r2.teacher;
			} else if (field == SUBJECT) {
				if (r1.subject != r2.subject)
					return r1.subject < r2.subject;
			} else if (field == ROOM) {
				if (r1.room != r2.room)
					return r1.room < r2.room;
			} else if (field == DAY) {
				if (r1.time.day != r2.time.day)
					return r1.time.day < r2.time.day;
			} else if (field == PERIOD) {
				if (r1.time.period != r2.time.period)
					return r1.time.period < r2.time.period;
			} else if (field == GROUP) {
				if (r1.group != r2.group)
					return r1.group < r2.group;
			}
		}
		return false;
	});

	std::vector<std::string> ans;
	for (const Record &rec : records) {
		ans.push_back("");
		for (Field field : q->fields()) {
			if (field == TEACHER)
				ans.back() += rec.teacher;
			else if (field == SUBJECT)
				ans.back() += rec.subject;
			else if (field == ROOM)
				ans.back() += std::to_string(rec.room);
			else if (field == DAY)
				ans.back() += std::to_string(rec.time.day);
			else if (field == PERIOD)
				ans.back() += std::to_string(rec.time.period);
			else if (field == GROUP)
				ans.back() += std::to_string(rec.group);
			ans.back() += "; ";
		}
	}
	_sessions[user].last_query = PRINT;
	result.set_protcode(PRINT_DATA);
	result.set_info(ans);
	return result;
}


QueryResult Database::shutdown(const UserId &user, const Query *query)
{
	assert((query == nullptr || query->type() == SHUTDOWN) && "Incorrect call of \'remove_all_users\'");
	QueryResult result;
	result.set_protcode(QUIT);
	result.set_servcode(SERVER_SHUTDOWN);
	_sessions[user].last_query = SHUTDOWN;
	return result;
}


const Database::Scripts& Database::scripts()
{
	static const Scripts ret{
		{STOP, &Database::remove_user},
		{SHUTDOWN, &Database::shutdown},
		{INSERT, &Database::insert},
		{REMOVE, &Database::remove},
		{SELECT, &Database::select},
		{RESELECT, &Database::reselect},
		{PRINT, &Database::print}};
	return ret;
}


/* -----------------------------------------PUBLIC METHODS--------------------------------------- */


void Database::from_file(const std::string &filename)
{
	std::ifstream fin;
	fin.open(filename);
	if (!fin.is_open())
		throw DatabaseExcFile("Database: cannot open the file!");
	Record record;
	while (fin >> record) {
		InsertQuery query = record;
		insert(0, &query); // это  id точно не занят
	}
	fin.close();
}


void Database::to_file(const std::string &filename) const
{
	std::ofstream fout;
	fout.open(filename);
	if (!fout.is_open())
		throw DatabaseExcFile("Database: cannot open the file!");
	for (int i = 0; i < NUM_OF_PERIODS * NUM_OF_DAYS; ++i)
		for (int j = 0; j <= NUM_OF_ROOMS; ++j)
			if (!_schedule[i][j].empty())
				fout << get_record({i, j}) << '\n';
	fout.close();
}


QueryResult Database::process_query(const UserId &user, const std::string &str)
{
	if (!_sessions.contains(user))
		throw DatabaseExcUser("User not registered!");
	QueryResult result;
	Query *query;
	try {
		query = Query::create_query(str);
	} catch (const QueryExc &e) {
		result.set_protcode(ERROR);
		result.set_info(e.what());
		result.set_servcode(SEND_INFO);
		return result;
	}
	QueryExecutor executor = scripts().at(query->type());
	result = (this->*executor)(user, query);
	delete query;
	return result;
}


bool Database::add_user(const UserId &user)
{
	if (_sessions.contains(user))
		return false;
	_sessions[user].last_query = VOID;
	return true;
}


QueryResult Database::remove_user(const UserId &user, const Query *query)
{
	assert((query == nullptr || query->type() == STOP) && "Incorrect call of \'remove_user\'");
	QueryResult result;
	if (_sessions.contains(user))
		_sessions.erase(user);
	result.set_protcode(QUIT);
	result.set_servcode(DISCONNECT_USER);
	return result;
}

