#pragma once
#include "Define.hpp"
#include "ClientSession.hpp"

#include <Windows.h>
#include <sqlext.h>
#include <iostream>
#include <string>
#include <mutex>


/*
* 1. DB Schema
*  Usercode (IDENTITY Ű���� ���. USERCODE int IDENTITY (1, 1) NOT NULL)
*  UserID (USERID VARCHAR(20) NOT NULL)
*  Password (PASSWORD VARCHAR(20) NOT NULL)
*  AuthserverNum (LASTAUTHSERVER int NOT NULL)
* 
* 2. VARCHAR
*  �̷������� CHAR�� ���������� ������ ���̽��� ���ٰ�� �ϳ�
*  ������ ü���ϱ� ����� �����̶�� ���� ����.
* 
* 3. DB HANDLE �� �����忡 �Ҵ� ??? - ��
*  - �� �κе� ť�װ� ��Ƽ������� ���� ������
* 
* 4. �Ҵ� ������ �� ��û���� �ϴ� �ͺ��� ��üǮ�� Ȱ���ϴ°� ����.
* 
* 5. �̸� ������ �������صΰ� �����ϴ� �������(SQLPrepare - SQLExcute) �����Ͽ� ȿ�������� ����� �� �ִ�.
* ���߿� ��ȸ�� �ȴٸ� ���־��� ������ �̸� �������صΰ� ����Ǯ�� ������ ���� ������ �����غ���.
*/

const bool DB_DEBUG = true;
const int DB_THREAD_NUM = 2;



class DataBase
{
public:

	bool Initialize()
	{
		// ����� ��� �ۼ�
		MakeReservedWordList();

		InitAuth();

		CreateThreads();

		return true;
	}

	bool End()
	{
		DestroyThreads();

		// �ش� AuthServer�� �����ϴ°� �ƴ� ���� ������ �ʱ�ȭ �� ����.
		InitAuth();

		return true;
	}

	// ȸ�������� ��û�ϴ� �Լ�.
	eReturnCode SignUp(std::string id_, std::string pw_, SQLHSTMT& hstmt_)
	{
		if (!CheckPWIsValid(pw_) || !CheckIDIsValid(id_))
		{
			return eReturnCode::SIGNUP_FAIL;
		}

		// INSERT ������ �ۼ�

		//SQLWCHAR* query = (SQLWCHAR*)(L"INSERT INTO LOGINDATA (ID, Password) VALUES (?, ?)");
		//SQLWCHAR* query = (SQLWCHAR*)(L"IF NOT EXISTS (SELECT 1 FROM LOGINDATA WHERE ID = ?) INSERT INTO LOGINDATA (ID, Password) VALUES (?, ?)");

		SQLWCHAR query[1024] = { 0 };
		sprintf_s((char*)query, 1024, "IF NOT EXISTS (SELECT 1 FROM LOGINDATA WHERE ID = '%s') "
			"BEGIN "
			"    INSERT INTO LOGINDATA (ID, Password) VALUES ('%s', '%s'); "
			"    SELECT 'S' AS Result; "
			"END "
			"ELSE "
			"BEGIN "
			"    SELECT 'F' AS Result; "
			"END", id_.c_str(), id_.c_str(), pw_.c_str());

		SQLPrepare(hstmt_, query, SQL_NTS);

		SQLRETURN retCode = SQLExecute(hstmt_);

		if (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO)
		{
			SQLLEN num_rows;
			SQLCHAR result_message[1024];
			retCode = SQLFetch(hstmt_);
			if (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO)
			{
				SQLGetData(hstmt_, 1, SQL_C_CHAR, result_message, sizeof(result_message), &num_rows);
				if (DB_DEBUG)
				{
					std::lock_guard<std::mutex> guard(debugMutex);
					std::cout << result_message << "\n";
				}

				if (!strcmp((const char*)result_message, "S"))
				{
					return eReturnCode::SIGNUP_SUCCESS;
				}
				// 'F' -> Failed to INSERT : Already In Use
				else
				{
					return eReturnCode::SIGNUP_ALREADY_IN_USE;
				}
			}
			// SQLFetch Failed
			else
			{
				std::cerr << "DB::SIGNUP : Failed to Fetch\n";
				return eReturnCode::SIGNUP_FAIL;
			}
		}
		else
		{
			std::cerr << "DB::SIGNUP : Failed to Execute\n";
			return eReturnCode::SIGNUP_FAIL;
		}
	}
	
	// �α��� �� �����ڵ带 �޾ƿ��� �Լ�
	long SignIn(std::string id_, std::string pw_, SQLHSTMT& hstmt_)
	{
		if (!CheckIDIsValid(id_) || !CheckPWIsValid(pw_))
		{
			return CLIENT_NOT_CERTIFIED;
		}

		SQLLEN cbParam1 = SQL_NTS;

		// SELECT ������ �ۼ�
		//SQLWCHAR* query = (SQLWCHAR*)L"SELECT (USERCODE, LASTAUTHSERVER, PASSWORD) FROM LOGINDATA WHERE USERID = ?";

		SQLWCHAR query[1024] = { 0 };

		sprintf_s((char*)query,1024, "IF EXISTS (SELECT 1 FROM LOGINDATA WHERE ID = '%s' AND Password = '%s' AND LASTAUTHSERVER = 0) "
			"BEGIN "
			"    UPDATE LOGINDATA SET LASTAUTHSERVER = %d WHERE ID = '%s' "
			"    SELECT USERCODE WHERE ID = '%s' AS Result; "
			"END "
			"ELSE "
			"BEGIN "
			"    SELECT 0 AS Result; "
			"END", id_.c_str(), pw_.c_str(), AUTHSERVER_NUM, id_.c_str(), id_.c_str());


		SQLPrepare(hstmt_, query, SQL_NTS);

		SQLRETURN retCode = SQLExecute(hstmt_);

		if (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO)
		{
			SQLLEN num_rows;
			long result = 0;
			retCode = SQLFetch(hstmt_);
			if (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO)
			{
				SQLGetData(hstmt_, 1, SQL_C_LONG, &result, sizeof(long), &num_rows);
				if (DB_DEBUG)
				{
					std::lock_guard<std::mutex> guard(debugMutex);
					std::cout << "SIGNIN RESULT : " << result << "\n";
				}

				// 0 -> Failed to UPDATE : INCORRECT OR ALREADY LOGGED IN
				if (result == CLIENT_NOT_CERTIFIED)
				{
					SQLCloseCursor(hstmt_);
					return CLIENT_NOT_CERTIFIED;
				}
				// USERCODE -> SUCCESS to Signin
				else
				{
					SQLCloseCursor(hstmt_);
					return result;
				}
			}
			// SQLFetch Failed
			else
			{
				std::cerr << "DB::SIGNUP : Failed to Fetch\n";
				SQLCloseCursor(hstmt_);
				return CLIENT_NOT_CERTIFIED;
			}
		}
		else
		{
			std::cerr << "DB::SIGNUP : Failed to Execute\n";
			SQLCloseCursor(hstmt_);
			return CLIENT_NOT_CERTIFIED;
		}
	}

	void PushReq(unsigned short clientIndex_, std::string id_, std::string pw_, eDBReqCode req_)
	{
		DBRequest* tmp = new DBRequest(clientIndex_, id_, pw_, req_);

		std::lock_guard<std::mutex> guard(mReqLock);

		ReqQueue.push(tmp);

		return;
	}

	DBResult* PopRes()
	{
		std::lock_guard<std::mutex> guard(mResLock);
		
		if (ResQueue.empty())
		{
			return nullptr;
		}

		DBResult* ret = ResQueue.front();
		ResQueue.pop();

		return ret;
	}

	

private:

	void CreateThreads()
	{
		mIsRun = true;

		for (int i = 0; i < DB_THREAD_NUM; i++)
		{
			DBThreads.emplace_back([this]() { DBThreadWork(); });
		}

		return;
	}

	void DestroyThreads()
	{
		mIsRun = false;

		
		for (auto& i : DBThreads)
		{
			if (i.joinable())
			{
				i.join();
			}
		}
		
		return;
	}

	// ID�� ��ȿ���� �Ǵ��Ѵ�.
	bool CheckIDIsValid(std::string id_)
	{
		if (!InjectionCheck(id_))
		{
			return false;
		}

		if (id_.size() < 6 || id_.size() > 10)
		{
			return false;
		}

		return true;
	}
	// Password�� ��ȿ���� �Ǵ��Ѵ�.
	bool CheckPWIsValid(std::string pw_)
	{
		if (!InjectionCheck(pw_))
		{
			return false;
		}

		if (pw_.size() < 6 || pw_.size() > 10)
		{
			return false;
		}

		return true;
	}

	// DB������ ����. Ư�����ڳ� SQL���� �����Ѵ�.
	bool InjectionCheck(const std::string str_)
	{
		// Ư����ȣ ����
		for (int i = 0; i < str_.size(); i++)
		{
			if (str_[i] >= '0' && str_[i] <= '9')
			{
				continue;
			}

			if (str_[i] >= 'A' && str_[i] <= 'Z')
			{
				continue;
			}

			if (str_[i] >= 'a' && str_[i] <= 'z')
			{
				continue;
			}

			return false;
		}

		// ����� ����
		
		for (std::string word : reservedWord)
		{
			if (IsStringContains(str_, word))
			{
				return false;
			}
		}

		return true;
	}
	// �������ڿ��� Ư�� ���ڿ��� ������ �ִ��� �Ǵ��Ѵ�.
	bool IsStringContains(std::string orgstr, std::string somestr)
	{
		size_t strsize1 = orgstr.size(), strsize2 = somestr.size();

		for (size_t i = 0; i < strsize1; i++)
		{
			// �� �̻� ã�� �� ����
			if (i + strsize2 > strsize1)
			{
				return false;
			}

			if (orgstr[i] == somestr[0])
			{
				// ���� ���ڿ��ΰ�
				bool check = true;

				for (size_t j = 1; j < strsize2; j++)
				{
					// ���� ���ڿ��� �ƴ�
					if (orgstr[i + j] != somestr[j])
					{
						check = false;
						break;
					}
				}

				if (check)
				{
					return true;
				}
			}
		}

		return false;
	}
	// ����� ��� �ۼ�
	void MakeReservedWordList()
	{
		
		reservedWord.clear();

		// ����� : OR, SELECT, INSERT, DELETE, UPDATE, CREATE, DROP, EXEC, UNION, FETCH, DECLARE, TRUNCATE
		reservedWord.push_back("OR");
		reservedWord.push_back("SELECT");
		reservedWord.push_back("INSERT");
		reservedWord.push_back("DELETE");
		reservedWord.push_back("UPDATE");
		reservedWord.push_back("CREATE");
		reservedWord.push_back("DROP");
		reservedWord.push_back("EXEC");
		reservedWord.push_back("UNION");
		reservedWord.push_back("FETCH");
		reservedWord.push_back("DECLARE");
		reservedWord.push_back("TRUNCATE");

		return;
	}

	// authserver ����ý� �ش� authserver�� ������ �ִ� �α��� ����� ���� �ʱ�ȭ�Ѵ�. (0����)
	void InitAuth()
	{
		SQLHENV henv;
		SQLHDBC hdbc;
		SQLHSTMT hstmt;

		SQLRETURN retcode;

		retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::InitAuth : ȯ�� �ڵ� �߱� ����\n";
			return;
		}

		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::InitAuth : ODBC ���� ���� ����\n";
			return;
		}

		retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::InitAuth : ���� �ڵ� �߱� ����\n";
			return;
		}

		std::wstring odbc = L"TestDB";
		std::wstring id = L"KIM";
		std::wstring pwd = L"kim123";

		SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (void*)5, 0);

		retcode = SQLConnect(hdbc, (SQLWCHAR*)odbc.c_str(), SQL_NTS, (SQLWCHAR*)id.c_str(), SQL_NTS, (SQLWCHAR*)pwd.c_str(), SQL_NTS);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::InitAuth : ���� ����\n";
			return;
		}

		retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

		if (retcode != SQL_SUCCESS)
		{
			std::cerr << "���� �ڵ� �Ҵ� ����";
			return;
		}

		SQLWCHAR* query = (SQLWCHAR*)L"UPDATE LOGINDATA SET LASTAUTHSERVER = 0 WHERE LASTAUTHSERVER = ?";

		long authnum = AUTHSERVER_NUM;

		retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &authnum, sizeof(authnum), nullptr);

		retcode = SQLExecDirect(hstmt, query, SQL_NTS);

		if (DB_DEBUG)
		{
			std::lock_guard<std::mutex> guard(debugMutex);
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				std::cout << "AUTH��� �ʱ�ȭ ����.\n";
			}
			else
			{
				std::cerr << "AUTH: ������ �ִ� ���� ����.\n";
			}
		}

		return;
	}

	void PushRes(unsigned short clientIndex_, eReturnCode res_)
	{
		DBResult* tmp = new DBResult(clientIndex_, res_);

		std::lock_guard<std::mutex> guard(mResLock);

		ResQueue.push(tmp);

		return;
	}

	void PushRes(unsigned short clientIndex_, eReturnCode res_, long usercode_)
	{
		DBResult* tmp = new DBResult(clientIndex_, res_, usercode_);

		std::lock_guard<std::mutex> guard(mResLock);

		ResQueue.push(tmp);

		return;
	}

	DBRequest* PopReq()
	{
		std::lock_guard<std::mutex> guard(mReqLock);

		if (ReqQueue.empty())
		{
			return nullptr;
		}

		DBRequest* ret = ReqQueue.front();
		ReqQueue.pop();

		return ret;
	}

	void DBThreadWork()
	{
		SQLHENV henv;
		SQLHDBC hdbc;
		SQLHSTMT hstmt;

		SQLRETURN retcode;

		retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::DBThreadWork : ȯ�� �ڵ� �߱� ����\n";
			return;
		}

		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::DBThreadWork : ODBC ���� ���� ����\n";
			return;
		}

		retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::DBThreadWork : ���� �ڵ� �߱� ����\n";
			return;
		}

		std::wstring odbc = L"TestDB";
		std::wstring id = L"KIM";
		std::wstring pwd = L"kim123";

		SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (void*)5, 0);

		retcode = SQLConnect(hdbc, (SQLWCHAR*)odbc.c_str(), SQL_NTS, (SQLWCHAR*)id.c_str(), SQL_NTS, (SQLWCHAR*)pwd.c_str(), SQL_NTS);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::DBThreadWork : ���� ����\n";
			return;
		}

		retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

		if (retcode != SQL_SUCCESS)
		{
			std::cerr << "DB::DBThreadWork : ���� �ڵ� �Ҵ� ����";
			return;
		}

		while (mIsRun)
		{
			DBRequest* req = PopReq();

			if (req != nullptr)
			{
				if (req->req == eDBReqCode::SIGNIN)
				{
					long iRet = SignIn(req->ID, req->PW, hstmt);

					if (iRet == CLIENT_NOT_CERTIFIED)
					{
						PushRes(req->clientIndex, eReturnCode::SIGNIN_FAIL);
					}
					// iRet = usercode
					else
					{
						PushRes(req->clientIndex, eReturnCode::SIGNIN_SUCCESS, iRet);
					}
				}
				if (req->req == eDBReqCode::SIGNUP)
				{
					eReturnCode eRet = SignUp(req->ID, req->PW, hstmt);

					PushRes(req->clientIndex, eRet);
				}
				// req->req == eDBReqCode::LOGOUT
				else
				{
					LogOut(req->clientIndex, hstmt);

					// ��� �뺸�� ���� �ʴ´�. (DB execute �����ϸ� �ݺ������� ������ϳ�?)
				}
				delete req;
				continue;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		SQLCancel(hstmt);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		SQLDisconnect(hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
		SQLFreeHandle(SQL_HANDLE_ENV, henv);

		return;
	}

	void LogOut(long usercode_, SQLHSTMT& hstmt_)
	{
		SQLWCHAR* query = (SQLWCHAR*)L"UPDATE LOGINDATA SET LASTAUTHSERVER = 0 WHERE USERCODE = ?";

		SQLRETURN retcode = SQLBindParameter(hstmt_, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &usercode_, sizeof(usercode_), nullptr);

		retcode = SQLExecDirect(hstmt_, query, SQL_NTS);

		if (DB_DEBUG)
		{
			std::lock_guard<std::mutex> guard(debugMutex);
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				std::cout << "LogOut : " << usercode_ << "\n";
			}
			else
			{
				std::cerr << "DataBase::LogOut : Failed to LogOut.\n";
			}
		}

		return;
	}

	// ����� ��� (DB������ ������)
	std::vector<std::string> reservedWord;
	
	// DB�� ����� ������
	std::vector<std::thread> DBThreads;

	// DBreq, DBres queue
	DBRequestQueue ReqQueue;
	DBResultQueue ResQueue;

	// DBreq, DBres queue mutex
	std::mutex mReqLock;
	std::mutex mResLock;

	bool mIsRun = false;

};