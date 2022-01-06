
/*   Copyright [2022] [wuzhigaoem@gmail.com]
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef _MYSQL_HEADER_H_
#define _MYSQL_HEADER_H_

#include <mysql/mysql.h>

#include <string>
#include <functional>

#include <list>

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace database {

#define MY_BIND_FIELD(var, index, type, name, len) var.bind[index].buffer_type = type; \
	var.bind[index].buffer = (char *)&(var.name); \
	if (len > 0) { var.bind[index].buffer_length= len; } \
	var.bind[index].is_null = &(var.is_null[index]); \
	var.bind[index].length = &(var.length[index]); \
	var.bind[index].error = &(var.error[index])

	class MySQL
	{
	public:
		template <typename int Columns>
		struct MyBind
		{
			MYSQL_BIND    bind[Columns];
			unsigned long length[Columns];
			my_bool       is_null[Columns];
			my_bool       error[Columns];
		};

	public:
		typedef struct _MyResult 
		{
			int error_no = 0;
			std::string error_str;
		} MyResult;

	public:
		// format year-month-day hour[24]:minute:second
		static void stringToDatetime(const std::string& str, MYSQL_TIME& datatime);

	public:
		explicit MySQL(const std::string& host, const std::string& user, const std::string& passwd, const std::string& db, unsigned int port, int connection_timeout = 5, char reconnect_auto = 1, int64_t heart_beat_interval = 60);
		~MySQL();

		MyResult Connect();
		void Connect(const std::function<void(const MyResult&)>& connect_callback);


		MyResult Query(const std::string& sql, MYSQL_RES** result, my_ulonglong& rows);
		void Query(const std::string& sql, const std::function<void(MYSQL_RES*, my_ulonglong, const MyResult&)>& callback);

		MyResult StatementQuery(const std::string& sql, MYSQL_STMT** stmt, my_ulonglong& rows, const std::function<void(MYSQL_STMT*, MyResult&)>& bind_param, const std::function<void(MYSQL_STMT*, MyResult&)>& bind_result);
		void StatementQuery(const std::string& sql, const std::function<void(MYSQL_STMT*, MyResult&)>& bind_param, const std::function<void(MYSQL_STMT*, MyResult&)>& bind_result, const std::function<void(MYSQL_STMT*, my_ulonglong, const MyResult&)>& callback);
		MyResult StatementExecute(const std::string& sql, MYSQL_STMT** stmt, my_ulonglong& rows, const std::function<void(MYSQL_STMT*, MyResult&)>& bind_param);
		void StatementExecute(const std::string& sql, const std::function<void(MYSQL_STMT*, MyResult&)>& bind_param, const std::function<void(MYSQL_STMT*, my_ulonglong, const MyResult&)>& callback);

		void Disconnect();

	private:
		void reset();
		void work();

	private:
		std::string _host;
		std::string _username;
		std::string _password;
		std::string _database;
		unsigned int _port;

	private:
		int _connection_timeout;
		char _reconnect_auto;
		int64_t _heart_beat_interval;

	private:
		bool _thread_inited;

	private:
		MYSQL* _mysql;

	private:
		std::function<void(const MyResult&)> _connect_callback;

	private:
		typedef struct _AsynOp
		{
			std::string sql;

			std::function<void(MYSQL_STMT*, MyResult&)> bind_param;
			std::function<void(MYSQL_STMT*, MyResult&)> bind_result;
			std::function<void(MYSQL_STMT*, my_ulonglong, const MyResult&)> stmt_callback;

			std::function<void(MYSQL_RES*, my_ulonglong, const MyResult&)> query_callback;

			_AsynOp()
				: sql(), bind_param(), bind_result(), stmt_callback()
				, query_callback()
			{
			}

			_AsynOp(const std::string& sql_, const std::function<void(MYSQL_STMT*, MyResult&)>& bind_param_, const std::function<void(MYSQL_STMT*, MyResult&)>& bind_result_, const std::function<void(MYSQL_STMT*, my_ulonglong, const MyResult&)>& stmt_callback_)
				: sql(sql_), bind_param(bind_param_), bind_result(bind_result_), stmt_callback(stmt_callback_)
				, query_callback()
			{
			}

			_AsynOp(const std::string& sql_, const std::function<void(MYSQL_RES*, my_ulonglong, const MyResult&)>& query_callback_)
				: sql(sql_), bind_param(), bind_result(), stmt_callback()
				, query_callback(query_callback_)
			{
			}

			_AsynOp(const std::string& sql_)
				: sql(sql_), bind_param(), bind_result(), stmt_callback()
				, query_callback()
			{
			}
		} AsynOp;

		std::thread _worker;
		std::atomic_bool _running;
		std::mutex _operations_locker;
		std::condition_variable _operations_condition;
		std::list<AsynOp> _operations;

	private:
		MySQL() = delete;

		MySQL(MySQL& rhs) = delete;
		MySQL& operator=(MySQL& rhs) = delete;

		MySQL(MySQL&& rhs) = delete;
		MySQL& operator=(MySQL&& rhs) = delete;
	};
};

#endif
