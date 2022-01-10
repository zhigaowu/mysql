

#include "MySQL.h"

#include <iostream>

database::MySQL::MyResult query_data(database::MySQL& mysql, const char* sql)
{
	database::MySQL::MyResult res;
	do 
	{
		MYSQL_RES* result = nullptr;
		my_ulonglong rows = 0;

		res = mysql.Query(sql, &result, rows);
		if (res.error_no != 0)
		{
			break;
		}

		if (result)
		{
			std::cout << "----------- " << rows << " -----------" << std::endl;
			MYSQL_ROW row;
			unsigned int i;
			unsigned int num_fields = mysql_num_fields(result);

			while ((row = mysql_fetch_row(result)))
			{
				for (i = 0; i < num_fields; i++)
				{
					std::cout << row[i] << ", ";
				}
				std::cout << std::endl;
			}

			mysql_free_result(result);
		}
	} while (false);
	return res;
}

database::MySQL::MyResult test_transaction_sync(int argc, char** argv, database::MySQL& mysql)
{
	database::MySQL::MyResult res = mysql.Connect();
	do
	{
		if (res.error_no != 0)
		{
			break;
		}

		res = query_data(mysql, "select FROM_UNIXTIME(UNIX_TIMESTAMP(NOW())) FROM DUAL");
		if (res.error_no != 0)
		{
			break;
		}

		res = mysql.BeginTransaction();
		if (res.error_no != 0)
		{
			break;
		}

		MYSQL_RES* result = nullptr;
		my_ulonglong rows = 0;
		res = mysql.Query("INSERT INTO file_item (sink_id, path, create_time, stop_time, state) VALUES(2, 'path/2022/01/10/23212315234.mp4', FROM_UNIXTIME(UNIX_TIMESTAMP(NOW())), FROM_UNIXTIME(UNIX_TIMESTAMP(NOW() + 30)), 'success')", &result, rows);
		if (res.error_no != 0)
		{
			break;
		}

		std::this_thread::sleep_for(std::chrono::seconds(3));

		res = mysql.Query("INSERT INTO file_item (sink_id, path, create_time, stop_time, state) VALUES(2, 'path/2022/01/10/23212315234.mp4', FROM_UNIXTIME(UNIX_TIMESTAMP(NOW())), FROM_UNIXTIME(UNIX_TIMESTAMP(NOW() + 30)), 'success')", &result, rows);
		if (res.error_no != 0)
		{
			break;
		}

		res = mysql.Query("delete from file_item where id = 10", &result, rows);
		if (res.error_no != 0)
		{
			break;
		}

		res = mysql.CommitTransaction();
		//res = mysql.RollbackTransaction();
		if (res.error_no != 0)
		{
			break;
		}

	} while (false);

	mysql.Disconnect();

	return res;
}
