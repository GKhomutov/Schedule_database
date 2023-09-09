#include "generator.h"

using namespace std;

random_device Generator::_rd;
mt19937 Generator::_rng(Generator::_rd());

template <class IntType>
IntType Generator::get_int(IntType min, IntType max)
{
	std::uniform_int_distribution<IntType> distrib(min, max);
	return distrib(_rng);
}

Generator::Generator(const string &teachers_filename, const string &subjects_filename)
{
	ifstream in_t(teachers_filename), in_s(subjects_filename);
	if (!in_t.is_open() || !in_s.is_open())
		throw GeneratorExc("Cannot open file in constructor!");
	string str;
	while (getline(in_t, str))
		_teachers.push_back(str);
	while (getline(in_s, str))
		_subjects.push_back(str);
	in_t.close(), in_s.close();
}

vector<string> Generator::get_insert(size_t num) const
{
	vector<string> result(num);
	for (size_t i = 0; i < num; ++i)
	{
		size_t t = get_int<size_t>(0, _teachers.size() - 1);
		size_t s = get_int<size_t>(0, _subjects.size() - 1);
		int r = get_int<>(1, NUM_OF_ROOMS);
		int	d = get_int<>(1, NUM_OF_DAYS);
		int p = get_int<>(1, NUM_OF_PERIODS);
		int g = get_int<>(1, NUM_OF_GROUPS);
		result[i] = "insert teacher=" + _teachers[t] + " subject=" + _subjects[s] +
			" room=" + to_string(r) + " day=" + to_string(d) + " period=" + to_string(p) +
			" group=" + to_string(g);
	}
	return result;
}

vector<string> Generator::get_conditional(size_t num, vector<string> commands) const
{
	vector<string> result(num);
	for (size_t i = 0; i < num; ++i)
	{
		size_t c = get_int<size_t>(0, commands.size() - 1);
		size_t t = get_int<size_t>(0, _teachers.size() - 1);
		size_t s = get_int<size_t>(0, _subjects.size() - 1);
		int r[2] = { get_int<>(1, NUM_OF_ROOMS), get_int<>(1, NUM_OF_ROOMS) };
		int d[2] = { get_int<>(1, NUM_OF_DAYS), get_int<>(1, NUM_OF_DAYS) };
		int p[2] = { get_int<>(1, NUM_OF_PERIODS), get_int<>(1, NUM_OF_PERIODS) };
		int g[2] = { get_int<>(1, NUM_OF_GROUPS), get_int<>(1, NUM_OF_GROUPS) };
		string teacher, subject, room, day, period, group;
		teacher = _teachers[t];
		subject = _subjects[s];
		size_t x = get_int<size_t>(0, teacher.length());
		size_t y = get_int<size_t>(0, subject.length());
		teacher = teacher.substr(0, x) + '*';
		subject = subject.substr(0, y) + '*';
		room = to_string(r[0]) + "-" + to_string(r[1]);
		day = to_string(d[0]) + "-" + to_string(d[1]);
		period = to_string(p[0]) + "-" + to_string(p[1]);
		group = to_string(g[0]) + "-" + to_string(g[1]);		

		stringstream candidate(" teacher=" + teacher + " subject=" + subject +
			" room=" + room + " day=" + day + " period=" + period + " group=" + group);
		result[i] = commands[c];
		for (size_t j = 0; j < Field_Vocabulary.size(); ++j) {
			int k = get_int<>(0, 3);
			string cond;
			candidate >> cond;
			if (k == 0)
				continue;
			result[i] += " " + cond;
		}
	}
	return result;
}

vector<string> Generator::get_print(size_t num) const
{
	vector<string> result(num);
	for (size_t i = 0; i < num; ++i)
	{
		vector<pair<string, Field>> fields, sortby;
		size_t f = get_int<size_t>(0, Field_Vocabulary.size());
		size_t s = get_int<size_t>(0, Field_Vocabulary.size());
		sample(Field_Vocabulary.begin(), Field_Vocabulary.end(), back_inserter(fields), f, _rng);
		sample(Field_Vocabulary.begin(), Field_Vocabulary.end(), back_inserter(sortby), s, _rng);

		result[i] = "print";
		for (const auto &p : fields)
			result[i] += " " + p.first;
		result[i] += " sort";
		for (const auto &p : sortby)
			result[i] += " " + p.first;
	}
	return result;
}

vector<string> Generator::get_mix(size_t num) const
{
	vector<string> result;
	size_t i = num / 4;
	size_t p = num / 5;
	size_t c = num - i - p;
	vector<string> insert = get_insert(i);
	vector<string> cond = get_conditional(c);
	vector<string> print = get_print(p);
	move(insert.begin(), insert.end(), back_inserter(result));
	move(cond.begin(), cond.end(), back_inserter(result));
	move(print.begin(), print.end(), back_inserter(result));
	shuffle(result.begin(), result.end(), _rng);
	return result;
}
