

#include "MySQL.h"

#include <iostream>

#define PATH_SIZE 128
#define DESC_SIZE 256


database::MySQL::MyResult test_stmt_async(int argc, char** argv, database::MySQL& mysql)
{
	database::MySQL::MyResult res;
	
	mysql.Connect([&res](const database::MySQL::MyResult& conn_res) {
		std::cout << "database connect state: " << conn_res.error_str << "(" << conn_res.error_no << ")" << std::endl;
		res = conn_res;
	});

	MYSQL_STMT* stmt = nullptr;
	my_ulonglong rows = 0;

	struct FileItem : database::MySQL::MyBind<6>
	{
		my_ulonglong  id;
		unsigned int  sink_id;
		char          path[PATH_SIZE];
		MYSQL_TIME    create_time, stop_time;
		char          state[DESC_SIZE];
	};

	struct FileParam : database::MySQL::MyBind<5>
	{
		unsigned int  sink_id;
		MYSQL_TIME    create_time, stop_time;
	};

	FileParam param;
	FileItem item;

	mysql.StatementQuery("SELECT id, sink_id, path, create_time, stop_time, state FROM file_item where sink_id = ? and ((create_time < ? and stop_time > ?) || (create_time >= ? and create_time < ?))", [&param](MYSQL_STMT* stmt, database::MySQL::MyResult& res) {
		memset(&param, 0, sizeof(param));

		param.sink_id = 1;
		database::MySQL::stringToDatetime("2021-12-27 14:25:57", param.create_time);		
		database::MySQL::stringToDatetime("2021-12-27 14:45:59", param.stop_time);

		MY_BIND_FIELD(param, 0, MYSQL_TYPE_LONG, sink_id, 0);

		MY_BIND_FIELD(param, 1, MYSQL_TYPE_DATETIME, create_time, 0);
		MY_BIND_FIELD(param, 2, MYSQL_TYPE_DATETIME, create_time, 0);

		MY_BIND_FIELD(param, 3, MYSQL_TYPE_DATETIME, create_time, 0);
		MY_BIND_FIELD(param, 4, MYSQL_TYPE_DATETIME, stop_time, 0);

		res.error_no = mysql_stmt_bind_param(stmt, param.bind);
	}, [&item](MYSQL_STMT* stmt, database::MySQL::MyResult& res) {
		memset(&item, 0, sizeof(item));

		MY_BIND_FIELD(item, 0, MYSQL_TYPE_LONGLONG, id, 0);

		MY_BIND_FIELD(item, 1, MYSQL_TYPE_LONG, sink_id, 0);

		MY_BIND_FIELD(item, 2, MYSQL_TYPE_STRING, path, sizeof(item.path));

		MY_BIND_FIELD(item, 3, MYSQL_TYPE_DATETIME, create_time, 0);

		MY_BIND_FIELD(item, 4, MYSQL_TYPE_DATETIME, stop_time, 0);

		MY_BIND_FIELD(item, 5, MYSQL_TYPE_STRING, state, sizeof(item.state));

		res.error_no = mysql_stmt_bind_result(stmt, item.bind);
	}, [&res, &item](MYSQL_STMT* stmt, my_ulonglong rows, const database::MySQL::MyResult& stmt_res) {
		if (stmt_res.error_no == 0)
		{
			std::cout << "----------- " << rows << " -----------" << std::endl;

			while ((res.error_no = mysql_stmt_fetch(stmt)) == 0 || res.error_no == MYSQL_DATA_TRUNCATED)
			{
				std::cout << item.id << ", "
					<< item.sink_id << ", "
					<< item.path << ", "
					<< item.create_time.year << "-" << item.create_time.month << "-" << item.create_time.day << " " << item.create_time.hour << ":" << item.create_time.minute << ":" << item.create_time.second << ", "
					<< item.stop_time.year << "-" << item.stop_time.month << "-" << item.stop_time.day << " " << item.stop_time.hour << ":" << item.stop_time.minute << ":" << item.stop_time.second << ", "
					<< item.state << std::endl;
			}
		}
		else
		{
			res = stmt_res;
		}

		std::cout << mysql_stmt_error(stmt) << "(" << res.error_no << ")" << std::endl;
	});

	std::this_thread::sleep_for(std::chrono::hours(1));

	mysql.Disconnect();

	return res;
}
