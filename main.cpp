

#include "MySQL.h"

#include <iostream>

extern database::MySQL::MyResult test_query_sync(int argc, char** argv, database::MySQL& mysql);
extern database::MySQL::MyResult test_query_async(int argc, char** argv, database::MySQL& mysql);
extern database::MySQL::MyResult test_stmt_sync(int argc, char** argv, database::MySQL& mysql);
extern database::MySQL::MyResult test_stmt_async(int argc, char** argv, database::MySQL& mysql);
extern database::MySQL::MyResult test_transaction_sync(int argc, char** argv, database::MySQL& mysql);

int main(int argc, char** argv)
{
	int res = 0;
	do
	{
		if (argc < 6)
		{
			std::cout << "usage: " << argv[0] << "casename host username password database port" << std::endl;
			std::cin >> argc;
			break;
		}

		if ((res = mysql_server_init(argc, argv, NULL)) < 0)
		{
			break;
		}

		database::MySQL mysql(argv[2], argv[3], argv[4], argv[5], atoi(argv[6]));

		database::MySQL::MyResult mysql_res;
		if (strcmp(argv[1], "query_sync") == 0)
		{
			mysql_res = test_query_sync(argc, argv, mysql);
		}
		else if (strcmp(argv[1], "query_async") == 0)
		{
			mysql_res = test_query_async(argc, argv, mysql);
		}
		else if (strcmp(argv[1], "trans_sync") == 0)
		{
			mysql_res = test_transaction_sync(argc, argv, mysql);
		}
		else if (strcmp(argv[1], "stmt_sync") == 0)
		{
			mysql_res = test_stmt_sync(argc, argv, mysql);
		}
		else if (strcmp(argv[1], "stmt_async") == 0)
		{
			mysql_res = test_stmt_async(argc, argv, mysql);
		}
		else
		{
			std::cout << "case " << argv[1] << " no defined" << std::endl;
		}

		if (mysql_res.error_no != 0)
		{
			std::cout << mysql_res.error_str << std::endl;
		}

		mysql_server_end();
	} while (false);

	std::cin >> argc;

	return res;
}
