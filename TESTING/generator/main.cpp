
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "generator.h"

using namespace std;

int main(void)
{
	Generator gen("info/teachers.txt", "info/subjects.txt");
	ofstream res1("../tests/insert.txt");
	ofstream res2("../tests/mix.txt");
	size_t n;

	cout << "How many inserts?\n";
	cin >> n;
	vector<string> insert = gen.get_insert(n);
	for (const string &s : insert)
		res1 << s << '\n';
	res1 << "stop\n";

	cout << "How many mix?\n";
	cin >> n;
	vector<string> mix = gen.get_mix(n);
	for (const string &s : mix)
		res2 << s << '\n';
	res2 << "stop\n";

	res1.close(), res2.close();
	return 0;
}
