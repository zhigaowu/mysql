

#include "MySQL.h"

#include <iostream>

database::MySQL::MyResult test_query_async(int argc, char** argv, database::MySQL& mysql)
{
	database::MySQL::MyResult res;

	mysql.Connect([&res](const database::MySQL::MyResult& conn_res) {
		std::cout << "database connect state: " << conn_res.error_str << "(" << conn_res.error_no << ")" << std::endl;
		res = conn_res;
	});

	mysql.Query("select * from file_item", [&res](MYSQL_RES* result, my_ulonglong rows, const database::MySQL::MyResult& conn_res) {
		if (conn_res.error_no == 0)
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
		} 
		else
		{
			res = conn_res;
		}
	});


	std::this_thread::sleep_for(std::chrono::hours(1));

	mysql.Disconnect();

	return res;
}
