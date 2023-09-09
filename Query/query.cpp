#include "query.h"

const Factory<Query, std::string>& Query::factory()
{
	static const Factory<Query, std::string> ret(
	[](){
		Factory<Query, std::string> tmp;
		tmp.add<StopQuery>("STOP");
		tmp.add<ShutdownQuery>("SHUTDOWN");
		tmp.add<InsertQuery>("INSERT");
		tmp.add<RemoveQuery>("REMOVE");
		tmp.add<SelectQuery>("SELECT");
		tmp.add<ReselectQuery>("RESELECT");
		tmp.add<PrintQuery>("PRINT");
		return tmp;
	}());
	return ret;
}

Field Query::recognize_field(std::string name) const
{
	std::transform(name.begin(), name.end(), name.begin(), toupper);
	auto it = Field_Vocabulary.find(name);
	if (it == Field_Vocabulary.end())
		throw QueryExcSyntax("No such field exists!");
	return it->second;
}

Query* Query::create_query(const std::string &str)
{
	std::stringstream ss(str);
	std::string command;
	ss >> command;
	std::transform(command.begin(), command.end(), command.begin(), toupper);
	if (!factory().is_registered(command))
		throw QueryExcSyntax("No such command exists!");
	Query *res = factory().create(command);
	try {
		res->parse(ss);
	} catch (...) {
		delete res;
		throw;
	}
	return res;
}

void StopQuery::parse(std::istream &is)
{
	if (is.get() != EOF)
		throw QueryExcSyntax("\'stop\' command must be one word!");
}

void ShutdownQuery::parse(std::istream &is)
{
	if (is.get() != EOF)
		throw QueryExcSyntax("\'shutdown\' command must be one word!");
}

int ConditionalQuery::recognize_int(Field field, const std::string &text, BoundaryType bt) const
{
	if (text == "*") {
		if (bt == SINGLE)
			throw QueryExcSyntax("Your query is syntactically incorrect!");
		if (field == ROOM)
			return bt == LEFT ? 0 : NUM_OF_ROOMS;
		if (field == DAY)
			return bt == LEFT ? 1 : NUM_OF_DAYS;
		if (field == PERIOD)
			return bt == LEFT ? 1 : NUM_OF_PERIODS;
		if (field == GROUP)
			return bt == LEFT ? 0 : NUM_OF_GROUPS;
	}

	if (std::find_if(text.begin(), text.end(), [](char c)
					 { return !std::isdigit(c); }) != text.end())
		throw QueryExcSyntax("Your query is syntactically incorrect!");
	int x = std::stoi(text);
	if ((field == ROOM && x > NUM_OF_ROOMS) ||
		(field == DAY && (x == 0 || x > NUM_OF_DAYS)) ||
		(field == PERIOD && (x == 0 || x > NUM_OF_PERIODS)) ||
		(field == GROUP && x > NUM_OF_GROUPS))
		throw QueryExcValue("Your query contains an invalid number!");
	return x;
}

Condition ConditionalQuery::merge_conditions(const Condition &c1, const Condition &c2) const
{
	Condition ans;
	ans.field = c1.field;
	if (ans.field == TEACHER || ans.field == SUBJECT)
	{
		const auto &name1 = std::get<std::string>(c1.value);
		const auto &name2 = std::get<std::string>(c2.value);
		if (c1.relation == EQUAL) {
			ans.relation = EQUAL;
			if (c2.relation == EQUAL)
				ans.value = (name1 == name2 ? name1 : "#"); // # - точно не имя
			else
				ans.value = (name1.starts_with(name2) ? name1 : "#");
		} else if (c2.relation == EQUAL) {
			ans.relation = EQUAL;
			ans.value = (name2.starts_with(name1) ? name2 : "#");
		} else {
			ans.relation = BEGIN;
			if (name1.starts_with(name2))
				ans.value = name1;
			else if (name2.starts_with(name1))
				ans.value = name2;
			else
				ans.value = "#";
		}
	} 
	else 
	{
		std::pair<int, int> r1 = c1.get_range(), r2 = c2.get_range();		
		ans.relation = RANGE;
		ans.value = std::make_pair(std::max(r1.first, r2.first), std::min(r1.second, r2.second));
	}
	return ans;
}

ConditionalQuery::ConditionalQuery(const std::vector<Condition> &conditions) : _conditions(conditions)
{
	std::sort(_conditions.begin(), _conditions.end(), cmp);
}

void ConditionalQuery::parse(std::istream &is)
{
	if (is.get() == EOF)
		throw QueryExcSyntax("Your query is syntactically incorrect!");
	std::string tok;
	while (is >> tok)
	{
		Condition cond;
		std::string::size_type separator = tok.find('=');
		if (separator == std::string::npos || separator == tok.length() - 1)
			throw QueryExcSyntax("Your query is syntactically incorrect!");
		std::string field_name(tok, 0, separator);
		std::string cond_text(tok, separator + 1);
		cond.field = recognize_field(field_name);

		if (cond.field == TEACHER || cond.field == SUBJECT) {
			if (cond_text.back() == '*') {
				cond.relation = BEGIN;
				cond_text.pop_back();
			} else {
				cond.relation = EQUAL;
			}
			if (std::find_if(cond_text.begin(), cond_text.end(), [](char c) {
						return !(std::isalpha(c) || c == '.' || c == '-');
					}) != cond_text.end())
				throw QueryExcSyntax("Your query is syntactically incorrect!");
			cond.value = cond_text;
		}
		else {
			separator = cond_text.find('-');
			if (separator != std::string::npos && separator != 0 && separator != cond_text.length() - 1) {
				cond.relation = RANGE;
				std::string val1(cond_text, 0, separator);
				std::string val2(cond_text, separator + 1);
				int x = recognize_int(cond.field, val1, LEFT);
				int y = recognize_int(cond.field, val2, RIGHT);
				cond.value = std::make_pair(std::min(x, y), std::max(x, y));
			} else {
				cond.relation = EQUAL;
				cond.value = recognize_int(cond.field, cond_text, SINGLE);
			}
		}
		_conditions.push_back(cond);
	}
	
	std::sort(_conditions.begin(), _conditions.end(), cmp);
	for (size_t i = 0; i < _conditions.size() - 1; i++)
		if (_conditions[i].field == _conditions[i + 1].field)
			throw QueryExcSyntax("One field - one condition!");
}

ConditionalQuery& ConditionalQuery::operator*=(const ConditionalQuery &other)
{
	const std::vector<Condition> &conds1 = _conditions;
	const std::vector<Condition> &conds2 = other._conditions;
	std::vector<Condition> new_conds;
	size_t i1, i2;
	for (i1 = i2 = 0; i1 < conds1.size() && i2 < conds2.size(); /* void */) {
		Condition cond, c1 = conds1[i1], c2 = conds2[i2];
		if (c1.field == c2.field) {
			cond = merge_conditions(c1, c2);
			++i1, ++i2;
		} else if (cmp(c1, c2)) {
			cond = c1;
			++i1;
		} else {
			cond = c2;
			++i2;
		}
		new_conds.push_back(cond);
	}
	while (i1 < conds1.size())
		new_conds.push_back(conds1[i1++]);
	while (i2 < conds2.size())
		new_conds.push_back(conds2[i2++]);
	_conditions = new_conds;
	return *this;
}

InsertQuery::InsertQuery(const Record &record) 
{
	_conditions = {
		{TEACHER, EQUAL, record.teacher},
		{SUBJECT, EQUAL, record.subject},
		{ROOM, EQUAL, record.room},
		{DAY, EQUAL, record.time.day},
		{PERIOD, EQUAL, record.time.period},
		{GROUP, EQUAL, record.group}
	};
	std::sort(_conditions.begin(), _conditions.end(), cmp);
}

void InsertQuery::parse(std::istream &is)
{
	ConditionalQuery::parse(is);
	if (_conditions.size() < NUM_OF_FIELDS)
		throw QueryExcSyntax("All fields must be set!");
	for (const Condition &cond : _conditions)
		if (cond.relation != EQUAL)
			throw QueryExcSyntax("Invalid field format!");
}

void PrintQuery::parse(std::istream &is)
{
	if (is.get() == EOF)
		throw QueryExcSyntax("Your query is syntactically incorrect!");
	std::string tok;
	bool reading_sort = false;
	while (is >> tok)
	{
		std::transform(tok.begin(), tok.end(), tok.begin(), toupper);
		if (tok == "SORT") {
			if (reading_sort)
				throw QueryExcSyntax("Query can only include one 'sort' keyword");
			reading_sort = true;
		} else {
			if (reading_sort)
				_sortby.push_back(recognize_field(tok));
			else
				_fields.push_back(recognize_field(tok));
		}
	}
}

void QueryResult::send_int(int fd, int number)
{
	int bytes_sent = send(fd, &number, sizeof(number), MSG_WAITALL);
	if (bytes_sent != sizeof(number))
		throw QueryExcSend("Cannot send integer");
}

void QueryResult::send_str(int fd, const char *str)
{
	int len = strlen(str);
	send_int(fd, len);
	int bytes_sent = send(fd, str, len, MSG_WAITALL);
	if (bytes_sent != len)
		throw QueryExcSend("Cannot send string");
}

void QueryResult::send_result(int fd) const
{
	send_int(fd, _protcode);
	switch (_protcode)
	{
	case SUCCESS:
		break;
	case PRINT_DATA: {
		const auto &rows = std::get<std::vector<std::string>>(_info);
		int n = rows.size();
		send_int(fd, n);
		for (int i = 0; i < n; ++i)
			send_str(fd, rows[i].c_str());
		break;
	}
	case QUIT:
		break;
	case ERROR:
		send_str(fd, std::get<const char*>(_info));
		break;
	}
}
