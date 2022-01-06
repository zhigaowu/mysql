

#include "MySQL.h"

#include <iostream>

database::MySQL::MyResult test_query_sync(int argc, char** argv, database::MySQL& mysql)
{
	database::MySQL::MyResult res = mysql.Connect();
	do
	{
		if (res.error_no != 0)
		{
			break;
		}

		MYSQL_RES* result = nullptr;
		my_ulonglong rows = 0;

		res = mysql.Query("select * from file_item", &result, rows);
		if (res.error_no != 0)
		{
			break;
		}

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
	} while (false);

	mysql.Disconnect();

	return res;
}
