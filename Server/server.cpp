#include <sys/socket.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <netdb.h>
#include <unistd.h>
#include <cerrno>
#include <cctype>
#include <iostream>
#include <string>
#include <algorithm>
#include <exception>

#include "../Database/database.h"
#include "../TaskStructures/task_structures.h"

#define PORT 5555
#define QUEUE_SIZE 3		// размер очереди входящих запросов соединения
#define MAX_CONNECTIONS	10	// максимальное количество одновременных соединений

Database database;

pollfd act_set[MAX_CONNECTIONS + 1];
int num_set = 0;

/* Закрывает сокет, при этом корректирует счётчик цикла проверки сокетов. */
void closeSocket(int &index);
void closeAllSockets();
int readStrFromClient(int fd, std::string &str);

int main(void)
{
	int err, opt = 1;
	int sock, new_sock;
	struct sockaddr_in server;
	struct sockaddr_in client;

	/* Заполняем структуру адреса, на котором будет работать сервер. */
	server.sin_family = AF_INET; // IP
	server.sin_addr.s_addr = htonl(INADDR_ANY); // любой сетевой интерфейс
	server.sin_port = htons(PORT);	// избегаем проблем с порядком байт в записи числа

	/* Создаём канал для сетевого обмена, задаём семейство протоколов и конкретный протокол обмена. */
	sock = socket(PF_INET, SOCK_STREAM, 0); // TCP сокет
	if (sock < 0) {
		perror("Server cannot create socket");
		exit(EXIT_FAILURE);
	}

	/* Сокет будет использоваться без ожидания таймаута закрытия. */
	err = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
	if (err < 0) {
		perror("Server cannot set socket options");
		if (close(sock) < 0)
			perror("Server cannot close socket");
		exit(EXIT_FAILURE);
	}

	/* Привязываем локальный адрес server к сокету, т.е. присваиваем сокету имя */
	err = bind(sock, (struct sockaddr *)&server, sizeof(server));
	if (err < 0) {
		perror("Server cannot bind socket");
		if (close(sock) < 0)
			perror("Server cannot close socket");
		exit(EXIT_FAILURE);
	}
	
	/* Сокет sock используется для приёма соединений. Устанавливаем длину очереди. */
	err = listen(sock, QUEUE_SIZE);
	if (err < 0) {
		perror("Server listen failure");
		if (close(sock) < 0)
			perror("Server cannot close socket");
		exit(EXIT_FAILURE);
	}
	
	num_set = 1;				// изначально только слушающий сокет 
	act_set[0].fd = sock;
	act_set[0].events = POLLIN; // запрошенные события (INput - наличие данных для чтения)
	act_set[0].revents = 0;		// информация о произошедших событиях

	try {
		database.from_file("data.txt");
	} catch (const std::exception &e) {
		std::cout << e.what();
		closeAllSockets();
		exit(EXIT_FAILURE);
	}

	/* Бесконечный цикл проверки состояния сокетов. */
	std::cout << "Number of connections: " << num_set - 1 << std::endl;
	while (true)
	{
		int act_discr;	// количество описателей с обнаруженными событиями или ошибками
		act_discr = poll(act_set, num_set, -1); // ждём появления данных в каком-либо сокете
		if (act_discr < 0) {
			perror("Server poll failure");
			closeAllSockets();
			exit(EXIT_FAILURE);
		}
		
		for (int i = 0; i < num_set; ++i)
		{
			if (act_set[i].revents ^ POLLIN)
				continue;
			
			act_set[i].revents &= ~POLLIN;
			if (i == 0)
			{
				/* Фактически отвечаем на команду connect от клиента. */
				socklen_t size = sizeof(client);
				new_sock = accept(act_set[i].fd, (struct sockaddr*)&client, &size);
				if (sock < 0) {
					perror("Server accept failure");
					closeAllSockets();
					exit(EXIT_FAILURE);
				}
				if (num_set - 1 < MAX_CONNECTIONS) {
					act_set[num_set].fd = new_sock;
					act_set[num_set].events = POLLIN;
					act_set[num_set].revents = 0;
					database.add_user(act_set[num_set].fd);
					++num_set;
					std::cout << "Number of connections: " << num_set - 1 << std::endl;
				} else {
					QueryResult result;
					result.set_protcode(ERROR);
					result.set_info("Too many connections! Try later!");
					try {
						result.send_result(new_sock);
					} catch (const QueryExcSend &e) {
						perror(e.what());
					}
					std::cout << "Too many connections! The last client was not connected.\n";
					if (close(new_sock) < 0) {
						perror("Server cannot close socket");
						exit(EXIT_FAILURE);
					}
				}
			}
			else
			{
				/* Пришёл запрос в уже существующем соединении. */
				std::string query;
				err = readStrFromClient(act_set[i].fd, query);
				if (err < 0) {
					perror("Server cannot read string from client");
					database.remove_user(act_set[i].fd);
					closeSocket(i);
					continue;
				}
				QueryResult result = database.process_query(act_set[i].fd, query);
				try {
					result.send_result(act_set[i].fd);
				} catch (const QueryExcSend &e) {
					perror(e.what());
					database.remove_user(act_set[i].fd);
					closeSocket(i);
					continue;
				}
				ServerCode code = result.get_servcode();
				if (code == DISCONNECT_USER) {
					closeSocket(i);
					std::cout << "Number of connections: " << num_set - 1 << std::endl;
				}
				else if (code == SERVER_SHUTDOWN) {
					database.to_file("data.txt");
					closeAllSockets();
					std::cout << "Server shutdown\n";
					return 0;
				}
			}
		}
	}
}

void closeSocket(int &index)
{
	if (close(act_set[index].fd) < 0) {
		perror("Server cannot close socket");
		exit(EXIT_FAILURE);
	}
	if (index < num_set - 1) {
		act_set[index] = act_set[num_set - 1];
		--index;
	}
	--num_set;
}

void closeAllSockets()
{
	for (int i = 0; i < num_set; ++i)
		closeSocket(i);
}

int readStrFromClient(int fd, std::string &str)
{
	int len;
	int bytes_read;
	bytes_read = recv(fd, &len, sizeof(len), MSG_WAITALL); // считываем длину сообщения
	if (bytes_read != sizeof(len)) {
		return -1;
	}
	char *msg = new char[len];
	bytes_read = recv(fd, msg, len, MSG_WAITALL); // читаем сообщение целиком
	if (bytes_read != len) {
		delete[] msg;
		return -1;
	}
	str = std::string(msg, len);
	delete[] msg;
	return 0;
}
