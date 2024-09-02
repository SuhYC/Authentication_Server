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
*  Usercode (IDENTITY 키워드 사용. USERCODE int IDENTITY (1, 1) NOT NULL)
*  UserID (USERID VARCHAR(20) NOT NULL)
*  Password (PASSWORD VARCHAR(20) NOT NULL)
*  AuthserverNum (LASTAUTHSERVER int NOT NULL)
* 
* 2. VARCHAR
*  이론적으로 CHAR가 성능적으로 유리한 케이스가 많다고는 하나
*  실제로 체감하기 어려운 수준이라는 말이 많음.
* 
* 3. DB HANDLE 각 스레드에 할당 ??? - 완
*  - 이 부분도 큐잉과 멀티스레드로 따로 만들자
* 
* 4. 할당 해제를 매 요청마다 하는 것보다 객체풀을 활용하는게 좋다.
* 
* 5. 미리 쿼리를 컴파일해두고 실행하는 방식으로(SQLPrepare - SQLExcute) 재사용하여 효율적으로 사용할 수 있다.
* 나중에 기회가 된다면 자주쓰는 쿼리를 미리 컴파일해두고 쿼리풀을 가져다 쓰는 식으로 구성해보자.
*/

const bool DB_DEBUG = true;
const int DB_THREAD_NUM = 2;



class DataBase
{
public:

	bool Initialize()
	{
		// 예약어 목록 작성
		MakeReservedWordList();

		InitAuth();

		CreateThreads();

		return true;
	}

	bool End()
	{
		DestroyThreads();

		// 해당 AuthServer만 종료하는게 아닐 수도 있으니 초기화 후 종료.
		InitAuth();

		return true;
	}

	// 회원가입을 요청하는 함수.
	eReturnCode SignUp(std::string id_, std::string pw_, SQLHSTMT& hstmt_)
	{
		if (!CheckPWIsValid(pw_) || !CheckIDIsValid(id_))
		{
			return eReturnCode::SIGNUP_FAIL;
		}

		// INSERT 쿼리문 작성

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
	
	// 로그인 후 유저코드를 받아오는 함수
	long SignIn(std::string id_, std::string pw_, SQLHSTMT& hstmt_)
	{
		if (!CheckIDIsValid(id_) || !CheckPWIsValid(pw_))
		{
			return CLIENT_NOT_CERTIFIED;
		}

		SQLLEN cbParam1 = SQL_NTS;

		// SELECT 쿼리문 작성
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

	// ID가 유효한지 판단한다.
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
	// Password가 유효한지 판단한다.
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

	// DB인젝션 방지. 특수문자나 SQL예약어를 제거한다.
	bool InjectionCheck(const std::string str_)
	{
		// 특수기호 제거
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

		// 예약어 제거
		
		for (std::string word : reservedWord)
		{
			if (IsStringContains(str_, word))
			{
				return false;
			}
		}

		return true;
	}
	// 원본문자열이 특정 문자열을 가지고 있는지 판단한다.
	bool IsStringContains(std::string orgstr, std::string somestr)
	{
		size_t strsize1 = orgstr.size(), strsize2 = somestr.size();

		for (size_t i = 0; i < strsize1; i++)
		{
			// 더 이상 찾을 수 없음
			if (i + strsize2 > strsize1)
			{
				return false;
			}

			if (orgstr[i] == somestr[0])
			{
				// 같은 문자열인가
				bool check = true;

				for (size_t j = 1; j < strsize2; j++)
				{
					// 같은 문자열이 아님
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
	// 예약어 목록 작성
	void MakeReservedWordList()
	{
		
		reservedWord.clear();

		// 예약어 : OR, SELECT, INSERT, DELETE, UPDATE, CREATE, DROP, EXEC, UNION, FETCH, DECLARE, TRUNCATE
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

	// authserver 재부팅시 해당 authserver가 가지고 있던 로그인 기록을 전부 초기화한다. (0으로)
	void InitAuth()
	{
		SQLHENV henv;
		SQLHDBC hdbc;
		SQLHSTMT hstmt;

		SQLRETURN retcode;

		retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::InitAuth : 환경 핸들 발급 실패\n";
			return;
		}

		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::InitAuth : ODBC 버전 설정 실패\n";
			return;
		}

		retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::InitAuth : 연결 핸들 발급 실패\n";
			return;
		}

		std::wstring odbc = L"TestDB";
		std::wstring id = L"KIM";
		std::wstring pwd = L"kim123";

		SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (void*)5, 0);

		retcode = SQLConnect(hdbc, (SQLWCHAR*)odbc.c_str(), SQL_NTS, (SQLWCHAR*)id.c_str(), SQL_NTS, (SQLWCHAR*)pwd.c_str(), SQL_NTS);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::InitAuth : 연결 실패\n";
			return;
		}

		retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

		if (retcode != SQL_SUCCESS)
		{
			std::cerr << "구문 핸들 할당 실패";
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
				std::cout << "AUTH기록 초기화 성공.\n";
			}
			else
			{
				std::cerr << "AUTH: 접속해 있던 세션 없음.\n";
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
			std::cerr << "DB::DBThreadWork : 환경 핸들 발급 실패\n";
			return;
		}

		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::DBThreadWork : ODBC 버전 설정 실패\n";
			return;
		}

		retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::DBThreadWork : 연결 핸들 발급 실패\n";
			return;
		}

		std::wstring odbc = L"TestDB";
		std::wstring id = L"KIM";
		std::wstring pwd = L"kim123";

		SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (void*)5, 0);

		retcode = SQLConnect(hdbc, (SQLWCHAR*)odbc.c_str(), SQL_NTS, (SQLWCHAR*)id.c_str(), SQL_NTS, (SQLWCHAR*)pwd.c_str(), SQL_NTS);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::DBThreadWork : 연결 실패\n";
			return;
		}

		retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

		if (retcode != SQL_SUCCESS)
		{
			std::cerr << "DB::DBThreadWork : 구문 핸들 할당 실패";
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

					// 결과 통보는 하지 않는다. (DB execute 실패하면 반복문으로 해줘야하나?)
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

	// 예약어 목록 (DB인젝션 방지용)
	std::vector<std::string> reservedWord;
	
	// DB와 통신할 스레드
	std::vector<std::thread> DBThreads;

	// DBreq, DBres queue
	DBRequestQueue ReqQueue;
	DBResultQueue ResQueue;

	// DBreq, DBres queue mutex
	std::mutex mReqLock;
	std::mutex mResLock;

	bool mIsRun = false;

};