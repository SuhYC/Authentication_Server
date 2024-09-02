#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h> // AcceptEx()
#include <mutex>
#include <vector>
#include <queue>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib") // acceptEx()

const unsigned short MAX_SOCKBUF = 100;
const unsigned short MAX_WORKERTHREAD = 12;

// 세션이 종료된 후 다시 사용할 수 있을 때까지의 시간 (단위 : 초)
const unsigned long long RE_USE_SESSION_WAIT_TIMESEC = 3;

// 토큰의 유효기간 (단위 : 초)
const unsigned long long LIFE_OF_TOKEN_SEC = 60;

// 여러개의 AUTH SERVER 중 몇번째의 서버인가
// 각 AUTH서버마다 다르게 설정해야함.
// 0은 설정하지 말것. 연결되지 않은 클라이언트를 0으로 설정함.
const long AUTHSERVER_NUM = 1;

std::mutex debugMutex;

enum class eIOOperation
{
	RECV,
	SEND,
	ACCEPT
};

struct stOverlappedEx
{
	WSAOVERLAPPED m_overlapped;
	unsigned short m_userIndex;
	WSABUF m_wsaBuf;
	eIOOperation m_eOperation;
};

enum class eReturnCode
{
	SIGNUP_FAIL,
	SIGNUP_ALREADY_IN_USE,
	SIGNUP_SUCCESS,
	SIGNIN_FAIL,
	SIGNIN_SUCCESS
};

enum class eDBReqCode
{
	SIGNUP,
	SIGNIN,
	LOGOUT
};

class DBRequest
{
public:
	DBRequest() = delete;

	DBRequest(unsigned long clientIndex_, std::string ID_, std::string PW_, eDBReqCode req_) : clientIndex(clientIndex_), ID(ID_), PW(PW_), req(req_) {}

	unsigned long clientIndex; // can be used as a usercode (only for logout)
	std::string ID;
	std::string PW;
	eDBReqCode req;
};

class DBResult
{
public:
	DBResult() = delete;
	DBResult(unsigned short clientIndex_, eReturnCode res_) : clientIndex(clientIndex_), res(res_)
	{
		usercode = 0;
	}
	DBResult(unsigned short clientIndex_, eReturnCode res_, long usercode_) : clientIndex(clientIndex_), res(res_), usercode(usercode_) {}

	unsigned short clientIndex;
	eReturnCode res;
	long usercode;
};