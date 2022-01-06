

#include "MySQL.h"

#include <iostream>

#define PATH_SIZE 128
#define DESC_SIZE 256

database::MySQL::MyResult test_stmt_sync(int argc, char** argv, database::MySQL& mysql)
{
	database::MySQL::MyResult res = mysql.Connect();
	do
	{
		if (res.error_no != 0)
		{
			break;
		}

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

		FileItem item;

		res = mysql.StatementQuery("SELECT id, sink_id, path, create_time, stop_time, state FROM file_item", &stmt, rows, [&item](MYSQL_STMT* stmt, database::MySQL::MyResult& res) {
			
		}, [&item](MYSQL_STMT* stmt, database::MySQL::MyResult& res) {
			memset(&item, 0, sizeof(item));

			MY_BIND_FIELD(item, 0, MYSQL_TYPE_LONGLONG, id, 0);
			
			MY_BIND_FIELD(item, 1, MYSQL_TYPE_LONG, sink_id, 0);

			MY_BIND_FIELD(item, 2, MYSQL_TYPE_STRING, path, sizeof(item.path));

			MY_BIND_FIELD(item, 3, MYSQL_TYPE_DATETIME, create_time, 0);

			MY_BIND_FIELD(item, 4, MYSQL_TYPE_DATETIME, stop_time, 0);

			MY_BIND_FIELD(item, 5, MYSQL_TYPE_STRING, state, sizeof(item.state));

			res.error_no = mysql_stmt_bind_result(stmt, item.bind);
		});

		std::cout << "----------- " << rows << " -----------" << std::endl;

		if (stmt)
		{
			while ((res.error_no = mysql_stmt_fetch(stmt)) == 0)
			{
				std::cout << item.id << ", "
					<< item.sink_id << ", "
					<< item.path << ", "
					<< item.create_time.year << "-" << item.create_time.month << "-" << item.create_time.day << " " << item.create_time.hour << ":" << item.create_time.minute << ":" << item.create_time.second << ", "
					<< item.stop_time.year << "-" << item.stop_time.month << "-" << item.stop_time.day << " " << item.stop_time.hour << ":" << item.stop_time.minute << ":" << item.stop_time.second << ", "
					<< item.state << std::endl;
			}

			mysql_stmt_free_result(stmt);
			mysql_stmt_close(stmt);
		}
	} while (false);
	
	mysql.Disconnect();

	return res;
}
