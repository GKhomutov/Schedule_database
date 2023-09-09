#ifndef TASK_STRUCTURES_H
#define TASK_STRUCTURES_H

#define NUM_OF_ROOMS 1000		// максимальный номер аудитории
#define NUM_OF_PERIODS 7		// максимальное количество пар в день
#define NUM_OF_DAYS 7			// количество дней
#define NUM_OF_GROUPS 699		// максимальный номер группы

#include <string>
#include <map>
#include <sstream>
#include <utility>
#include <variant>
#include <iostream>

/* Значение поля: число, диапазон или строка. */
using FieldValue = std::variant<int, std::pair<int, int>, std::string>;

/* Виды полей. */
typedef enum { TEACHER, SUBJECT, ROOM, DAY, PERIOD, GROUP, NUM_OF_FIELDS } Field;
const std::map<std::string, Field> Field_Vocabulary {
	{"TEACHER", TEACHER},
	{"SUBJECT", SUBJECT},
	{"ROOM", ROOM},
	{"DAY", DAY},
	{"PERIOD", PERIOD},
	{"GROUP", GROUP}
};

typedef enum
{
	SUCCESS = 0,	// Была выполнена одна из команд insert, remove, select, reselect
	PRINT_DATA = 1,	// Была выполнена команда print, нужно принять данные
	QUIT = 2,		// Была выполнена команда stop или shutdown, нужно прекратить работу
	ERROR = 3		// Возникла ошибка
} ProtocolCode;

typedef enum
{
	SEND_INFO,			// Отослать данные
	DISCONNECT_USER,	// Отключить клиента
	SERVER_SHUTDOWN		// Прекратить работу сервера
} ServerCode;

/* Виды отношений в условных запросах. */
typedef enum
{
	EQUAL, // =
	BEGIN, // = Хому*
	RANGE  // = 200-210 или = 200-*
} Relation;

/* Условие в запросе. */
struct Condition
{
	Field field;
	Relation relation;
	FieldValue value;

	std::pair<int, int> get_range() const;
};

/* Время определяется днём недели и парой. */
struct Time
{
	int day;
	int period;

	Time() {}
	Time(int d, int p) : day(d), period(p) {}
	Time(int timecode);
	operator int() const;
};

/* Запись в базе занных. */
struct Record {
	std::string teacher;
	std::string subject;
	int room;
	Time time;
	int group;
};
std::istream& operator>>(std::istream &s, Record &record);
std::ostream& operator<<(std::ostream &s, const Record &record);

/* Ячейка в расписании (разреженной таблице). */
struct ScheduleItem
{
	bool used;
	std::string teacher;
	std::string subject;
	int group;

	ScheduleItem() : used(false) {}
	void operator=(const Record &r);
	void clear() { used = false; }
	bool empty() const { return !used; }
};

struct SchedulePosition
{
	int timecode;
	int room;

	SchedulePosition() {}
	SchedulePosition(const Time &time, int r) : timecode(int(time)), room(r) { }
	bool operator==(const SchedulePosition &other) const;
};

#endif // TASK_STRUCTURES_H
