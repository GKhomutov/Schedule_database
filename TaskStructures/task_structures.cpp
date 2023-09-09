#include "task_structures.h"

std::pair<int, int> Condition::get_range() const
{
	std::pair<int, int> ans;
	if (relation == EQUAL)
		ans.first = ans.second = std::get<int>(value);
	else
		ans = std::get<std::pair<int, int>>(value);
	return ans;
}

Time::Time(int timecode)
{
	day = timecode / NUM_OF_PERIODS + 1;
	period = timecode % NUM_OF_PERIODS + 1;
}

Time::operator int() const
{
	return (day - 1) * NUM_OF_PERIODS + period - 1;
}

std::istream &operator>>(std::istream &s, Record &record)
{
	std::string str;
	std::getline(s, str);
	std::stringstream ss(str);
	std::getline(ss, record.teacher, ';');
	ss.ignore(1);
	std::getline(ss, record.subject, ';');
	ss >> record.room;
	ss.ignore(1);
	ss >> record.time.day;
	ss.ignore(1);
	ss >> record.time.period;
	ss.ignore(1);
	ss >> record.group;
	ss.ignore(1);
	return s;
}

std::ostream &operator<<(std::ostream &s, const Record &record)
{
	s << record.teacher << "; " << record.subject << "; " << record.room << "; "
	<< record.time.day << "; " << record.time.period << "; " << record.group << ";";
	return s;
}

void ScheduleItem::operator=(const Record &r)
{
	used = true;
	teacher = r.teacher;
	subject = r.subject;
	group = r.group;
}

bool SchedulePosition::operator==(const SchedulePosition &other) const
{
	return timecode == other.timecode && room == other.room;
}
