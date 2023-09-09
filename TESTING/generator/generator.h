#ifndef GENERATOR_H
#define GENERATOR_H

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <random>
#include <algorithm>
#include <utility>
#include "GeneratorExc.h"
#include "../../TaskStructures/task_structures.h"

class Generator
{
  private:
	std::vector<std::string> _teachers, _subjects;

	static std::random_device _rd;
	static std::mt19937 _rng;

	template<class IntType = int>
	static IntType get_int(IntType min, IntType max);

  public:
	Generator() = delete;
	Generator(const std::string &teachers_filename, const std::string &subjects_filename);
	std::vector<std::string> get_insert(size_t num) const;
	std::vector<std::string> get_conditional
		(size_t num, std::vector<std::string> commands = {"select", "reselect", "remove"}) const;
	std::vector<std::string> get_print(size_t num) const;
	std::vector<std::string> get_mix(size_t num) const;
};

#endif // GENERATOR_H
