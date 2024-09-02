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

// ������ ����� �� �ٽ� ����� �� ���� �������� �ð� (���� : ��)
const unsigned long long RE_USE_SESSION_WAIT_TIMESEC = 3;

// ��ū�� ��ȿ�Ⱓ (���� : ��)
const unsigned long long LIFE_OF_TOKEN_SEC = 60;

// �������� AUTH SERVER �� ���°�� �����ΰ�
// �� AUTH�������� �ٸ��� �����ؾ���.
// 0�� �������� ����. ������� ���� Ŭ���̾�Ʈ�� 0���� ������.
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