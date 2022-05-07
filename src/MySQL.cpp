#include "MySQL.h"

void database::MySQL::stringToDatetime(const std::string& str, MYSQL_TIME& datatime)
{
	sscanf(str.c_str(), "%d-%d-%d %d:%d:%d", &datatime.year, &datatime.month, &datatime.day, &datatime.hour, &datatime.minute, &datatime.second);
}

database::MySQL::MySQL(const std::string& host, const std::string& user, const std::string& passwd, const std::string& db, unsigned int port, int connection_timeout, char reconnect_auto, int64_t heart_beat_interval)
	: _host(host), _username(user), _password(passwd), _database(db), _port(port)
	, _connection_timeout(connection_timeout), _reconnect_auto(reconnect_auto), _heart_beat_interval(heart_beat_interval)
	, _thread_inited(false)
	, _mysql(nullptr)
	, _connect_callback()
	, _worker(), _running(false)
	, _operations_locker(), _operations_condition(), _operations()
{
}

database::MySQL::~MySQL()
{
}

static inline void mysql_error_result(database::MySQL::MyResult& res, MYSQL* mysql)
{
	res.error_no = mysql_errno(mysql);
	res.error_str = mysql_error(mysql);
}

database::MySQL::MyResult database::MySQL::Connect()
{
	MyResult res;
	do 
	{
		// initialize thread
		_thread_inited = (0 == mysql_thread_init());

		if (!(_mysql = mysql_init(_mysql)))
		{
			res.error_no = -1;
			res.error_str = "mysql initialize failed";
			break;
		}

		if ((res.error_no = mysql_options(_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &_connection_timeout)) != 0) 
		{
			mysql_error_result(res, _mysql);
			break;
		}

		if ((res.error_no = mysql_options(_mysql, MYSQL_OPT_RECONNECT, &_reconnect_auto)) != 0)
		{
			mysql_error_result(res, _mysql);
			break;
		}

		if (_mysql != mysql_real_connect(_mysql, /* MYSQL structure to use */
			_host.c_str(),				/* server hostname or IP address */
			_username.c_str(),          /* mysql user */
			_password.c_str(),          /* password */
			_database.c_str(),          /* default database to use, NULL for none */
			_port,                      /* port number, 0 for default */
			nullptr,                    /* socket file or named pipe name */
			CLIENT_FOUND_ROWS           /* connection flags */))
		{
			mysql_error_result(res, _mysql);
		}
	} while (false);

	return res;
}

void database::MySQL::Connect(const std::function<void(const MyResult&)>& callback)
{
	if (!_running)
	{
		_running = true;
		_worker = std::thread(&MySQL::work, this);
	}

	std::unique_lock<std::mutex> lg(_operations_locker);
	_connect_callback = callback;
	_operations.emplace_back("");
	_operations_condition.notify_one();
}

database::MySQL::MyResult database::MySQL::Query(const std::string& sql, MYSQL_RES** result, my_ulonglong& rows)
{
	MyResult res;
	do
	{
		// initialize parameters
		*result = nullptr;
		rows = 0;

		// query
		if ((res.error_no = mysql_real_query(_mysql, sql.c_str(), static_cast<unsigned long>(sql.size()))) != 0)
		{
			mysql_error_result(res, _mysql);
			break;
		}

		*result = mysql_store_result(_mysql);
		if (!(*result))
		{
			mysql_error_result(res, _mysql);
			break;
		}

		rows = mysql_num_rows(*result);
	} while (false);

	return res;
}

void database::MySQL::Query(const std::string& sql, const std::function<void(MYSQL_RES*, my_ulonglong, const MyResult&)>& callback)
{
	if (_running)
	{
		std::unique_lock<std::mutex> lg(_operations_locker);
		_operations.emplace_back(sql, callback);
		_operations_condition.notify_one();
	}
}

static inline void statement_error_result(database::MySQL::MyResult& res, MYSQL_STMT* stmt)
{
	res.error_no = mysql_stmt_errno(stmt);
	res.error_str = mysql_stmt_error(stmt);
}

database::MySQL::MyResult database::MySQL::StatementQuery(const std::string& sql, MYSQL_STMT** stmt, my_ulonglong& rows, const std::function<void(MYSQL_STMT*, MyResult&)>& bind_param, const std::function<void(MYSQL_STMT*, MyResult&)>& bind_result)
{
	MyResult res;
	do
	{
		// initialize parameters
		*stmt = mysql_stmt_init(_mysql);
		rows = 0;

		if (!(*stmt))
		{
			mysql_error_result(res, _mysql);
			break;
		}

		// prepare statement
		if ((res.error_no = mysql_stmt_prepare(*stmt, sql.c_str(), static_cast<unsigned long>(sql.size()))) != 0)
		{
			statement_error_result(res, *stmt);
			break;
		}

		// bind parameter
		bind_param(*stmt, res);
		if (res.error_no != 0)
		{
			statement_error_result(res, *stmt);
			break;
		}

		// execute statement
		if ((res.error_no = mysql_stmt_execute(*stmt)) != 0)
		{
			statement_error_result(res, *stmt);
			break;
		}

		// store result
		if ((res.error_no = mysql_stmt_store_result(*stmt)) != 0)
		{
			statement_error_result(res, *stmt);
			break;
		}

		rows = mysql_stmt_num_rows(*stmt);

		// bind result
		bind_result(*stmt, res);
		if (res.error_no != 0)
		{
			statement_error_result(res, *stmt);
			break;
		}
	} while (false);

	return res;
}

void database::MySQL::StatementQuery(const std::string& sql, const std::function<void(MYSQL_STMT*, MyResult&)>& bind_param, const std::function<void(MYSQL_STMT*, MyResult&)>& bind_result, const std::function<void(MYSQL_STMT*, my_ulonglong, const MyResult&)>& callback)
{
	if (_running)
	{
		std::unique_lock<std::mutex> lg(_operations_locker);
		_operations.emplace_back(sql, bind_param, bind_result, callback);
		_operations_condition.notify_one();
	}
}

database::MySQL::MyResult database::MySQL::StatementExecute(const std::string& sql, MYSQL_STMT** stmt, my_ulonglong& rows, const std::function<void(MYSQL_STMT*, MyResult&)>& bind_param)
{
	MyResult res;
	do
	{
		// initialize parameters
		*stmt = mysql_stmt_init(_mysql);
		rows = 0;

		if (!(*stmt))
		{
			mysql_error_result(res, _mysql);
			break;
		}

		// prepare statement
		if ((res.error_no = mysql_stmt_prepare(*stmt, sql.c_str(), static_cast<unsigned long>(sql.size()))) != 0)
		{
			statement_error_result(res, *stmt);
			break;
		}

		// bind parameter
		bind_param(*stmt, res);
		if (res.error_no != 0)
		{
			statement_error_result(res, *stmt);
			break;
		}

		// execute statement
		if ((res.error_no = mysql_stmt_execute(*stmt)) != 0)
		{
			statement_error_result(res, *stmt);
			break;
		}

		rows = mysql_stmt_affected_rows(*stmt);
	} while (false);

	return res;
}

void database::MySQL::StatementExecute(const std::string& sql, const std::function<void(MYSQL_STMT*, MyResult&)>& bind_param, const std::function<void(MYSQL_STMT*, my_ulonglong, const MyResult&)>& callback)
{
	if (_running)
	{
		std::unique_lock<std::mutex> lg(_operations_locker);
		_operations.emplace_back(sql, bind_param, nullptr, callback);
		_operations_condition.notify_one();
	}
}

database::MySQL::MyResult database::MySQL::BeginTransaction()
{
	MyResult res;
	// begin transaction
	if ((res.error_no = mysql_real_query(_mysql, "begin", static_cast<unsigned long>(5))) != 0)
	{
		mysql_error_result(res, _mysql);
	}

	return res;
}

void database::MySQL::BeginTransaction(const std::function<void(MYSQL_RES*, my_ulonglong, const MyResult&)>& callback)
{
	if (_running)
	{
		std::unique_lock<std::mutex> lg(_operations_locker);
		_operations.emplace_back("begin", callback);
		_operations_condition.notify_one();
	}
}

database::MySQL::MyResult database::MySQL::CommitTransaction()
{
	MyResult res;
	// commit transaction
	if ((res.error_no = mysql_real_query(_mysql, "commit", static_cast<unsigned long>(6))) != 0)
	{
		mysql_error_result(res, _mysql);
	}

	return res;
}

void database::MySQL::CommitTransaction(const std::function<void(MYSQL_RES*, my_ulonglong, const MyResult&)>& callback)
{
	if (_running)
	{
		std::unique_lock<std::mutex> lg(_operations_locker);
		_operations.emplace_back("commit", callback);
		_operations_condition.notify_one();
	}
}

database::MySQL::MyResult database::MySQL::RollbackTransaction()
{
	MyResult res;
	// rollback transaction
	if ((res.error_no = mysql_real_query(_mysql, "rollback", static_cast<unsigned long>(8))) != 0)
	{
		mysql_error_result(res, _mysql);
	}

	return res;
}

void database::MySQL::RollbackTransaction(const std::function<void(MYSQL_RES*, my_ulonglong, const MyResult&)>& callback)
{
	if (_running)
	{
		std::unique_lock<std::mutex> lg(_operations_locker);
		_operations.emplace_back("rollback", callback);
		_operations_condition.notify_one();
	}
}

void database::MySQL::Disconnect()
{
	_running = false;
	_operations_condition.notify_one();
	if (_worker.joinable())
	{
		_worker.join();
	}

	reset();
}

void database::MySQL::reset()
{
	if (_mysql)
	{
		mysql_close(_mysql);
		_mysql = nullptr;
	}

	if (_thread_inited) { mysql_thread_end(); }
}

void database::MySQL::work()
{
	MyResult res;
	while (_running)
	{
		AsynOp op;
		do 
		{
			std::unique_lock<std::mutex> lg(_operations_locker);
			_operations_condition.wait_for(lg, std::chrono::seconds(_heart_beat_interval), [this]() { return !_running || !_operations.empty(); });
			if (!_operations.empty())
			{
				op = _operations.front();
				_operations.pop_front();
			}
		} while (false);

		if (_running)
		{
			if ((op.query_callback || op.stmt_callback))
			{
				// check if connected
				if (!_mysql)
				{
					res.error_no = -1;
					res.error_str = "mysql is not connected";
					if (op.stmt_callback)
					{
						op.stmt_callback(nullptr, 0, res);
					}
					else
					{
						op.query_callback(nullptr, 0, res);
					}
					continue;
				}

				// call sql command
				my_ulonglong rows = 0;
				if (op.stmt_callback)
				{
					MYSQL_STMT* stmt = nullptr;
					if (op.bind_result)
					{
						res = StatementQuery(op.sql, &stmt, rows, op.bind_param, op.bind_result);
					}
					else
					{
						res = StatementExecute(op.sql, &stmt, rows, op.bind_param);
					}

					op.stmt_callback(stmt, rows, res);

					if (stmt)
					{
						mysql_stmt_free_result(stmt);
						mysql_stmt_close(stmt);
					}
				}
				else
				{
					MYSQL_RES* result = nullptr;

					res = Query(op.sql, &result, rows);

					op.query_callback(result, rows, res);

					if (result)
					{
						mysql_free_result(result);
					}
				}
			}
			else
			{
				if (!_mysql)
				{
					reset();
					res = Connect();
				}
				else
				{
					if ((res.error_no = mysql_ping(_mysql)) != 0)
					{
						mysql_error_result(res, _mysql);
					}
				}

				_connect_callback(res);
			}
		}
	}

	reset();
}


