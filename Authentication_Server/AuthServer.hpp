#include "IOCP.hpp"
#include "AES.hpp"
#include "RedisManager.hpp"
#include "Database.hpp"
#include "Base64.hpp"

/*
* 1. Redis Queueing
* if cli req token && cli authed
*	make token
*	set token to redis (queueing)
*	send token to cli
* 
* 2. DB Queueing
* if cli signup or signin
*	check RDBMS (queueing)
*	send res to cli
* (no token on this work.)
* 
* if cli want to get token
*	1. sign up
*	2. sign in
*	3. req token
* 
* 
* map�� �̿��� ����� client ��ȸ -> ���� auth server�� ����ϸ� ����� ����� �� �ְ� ������
* RDBMS�� �̿��� ����� client ��ȸ -> ���� auth server�� ����ȭ�Ͽ� ����� �� �ִ�.
* In Memory DB(Shard DB)�� �̿��� ����� client ��ȸ -> RDBMS���� ������
* 
* ���� �̹� ������Ʈ�� RDBMS�� ����ϵ��� �Ѵ�.. Redis�� ��ū���常!
*/

const bool AUTHSERVER_DEBUG = true;

class AuthServer : public IOCPServer
{
public:
	AuthServer() {};
	virtual ~AuthServer() = default;

	void Init(int bindPort_)
	{
		InitSocket(bindPort_);

		mRedis.Init();
		mDB.Initialize();

		return;
	}

	void Run(const unsigned short maxClient_)
	{
		mProcessThread = std::thread([this]() {ProcessSendPacket(); });
		mDBThread = std::thread([this]() {ProcessSendRes(); });

		StartServer(maxClient_);

		return;
	}

	void End()
	{
		mRedis.End();
		mDB.End();

		DestroyThread();

		AuthServer::mIsRun = false;

		if(mProcessThread.joinable())
		{
			mProcessThread.join();
		}

		if (mDBThread.joinable())
		{
			mDBThread.join();
		}

		return;
	}

private:
	virtual void OnConnect(const unsigned short clientIndex_) override
	{
		if (AUTHSERVER_DEBUG)
		{
			std::lock_guard<std::mutex> guard(debugMutex);
			std::cout << "[" << clientIndex_ << "] : Connected\n";
		}

		return;
	}

	virtual void OnReceive(const unsigned short clientIndex_, const unsigned long size_, char* pData_)
	{
		RecvPacketData recvPacket = { 0 };
		CopyMemory(&recvPacket, pData_, size_);

		long clientID = 0;
		std::string id, pw;
		ClientInfo* clientinfo = nullptr;

		switch (recvPacket.mPktHeader.pktid)
		{
			case Pktid::TOKEN_REQ:
				clientinfo = GetClientInfo(clientIndex_);
				clientID = clientinfo->GetUserID();

				if (clientID != CLIENT_NOT_CERTIFIED)
				{
					SendPacketData* msg = MakeSendPacket(clientIndex_, clientID);
					mRedis.PushReq(msg);
				}

				break;
			case Pktid::SIGNIN_REQ:
				id = recvPacket.mPktReq.id;
				pw = recvPacket.mPktReq.pw;

				if (AUTHSERVER_DEBUG)
				{
					std::lock_guard<std::mutex> guard(debugMutex);
					std::cout << "[ID] : " << id << "\n";
					std::cout << "[PW] : " << pw << "\n";
				}

				mDB.PushReq(clientIndex_, id, pw, eDBReqCode::SIGNIN);

				break;
			case Pktid::SIGNUP_REQ:
				id = recvPacket.mPktReq.id;
				pw = recvPacket.mPktReq.pw;

				if (AUTHSERVER_DEBUG)
				{
					std::lock_guard<std::mutex> guard(debugMutex);
					std::cout << "[ID] : " << id << "\n";
					std::cout << "[PW] : " << pw << "\n";
				}

				mDB.PushReq(clientIndex_, id, pw, eDBReqCode::SIGNUP);

				break;
			default:
				std::cerr << "[" << clientIndex_ << "] �� �� ���� ������ : " << pData_;
				if (AUTHSERVER_DEBUG)
				{
					std::lock_guard<std::mutex> guard(debugMutex);
					std::cout << "pktid: " << (int)recvPacket.mPktHeader.pktid
						<< ", totalsize: " << recvPacket.mPktHeader.TotalSize
						<< ", id: " << recvPacket.mPktReq.id
						<< ", pw: " << recvPacket.mPktReq.pw << "\n";
				}
				break;
		}

		return;
	}

	virtual void OnClose(const unsigned short clientIndex_)
	{
		ClientInfo* client = GetClientInfo(clientIndex_);

		long usercode = client->GetUserID();

		mDB.PushReq(usercode, "", "", eDBReqCode::LOGOUT);

		client->SetUserCode(CLIENT_NOT_CERTIFIED);

		if (AUTHSERVER_DEBUG)
		{
			std::lock_guard<std::mutex> guard(debugMutex);
			std::cout << "[" << clientIndex_ << "] : Close.\n";
		}

		return;
	}

	// �۽���Ŷ ó��.
	void ProcessSendPacket()
	{
		while (mIsRun)
		{
			SendPacketData* packetData = mRedis.GetRes();

			if (packetData != nullptr)
			{
				PushSendPacket(packetData);
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	}

	// SignIn, SignUp ��� �۽���Ŷ ó��
	void ProcessSendRes()
	{
		while (mIsRun)
		{
			DBResult* res = mDB.PopRes();

			if (res != nullptr)
			{
				Pktid pid;
				if (res->res == eReturnCode::SIGNIN_FAIL ||
					res->res == eReturnCode::SIGNIN_SUCCESS)
				{
					pid = Pktid::SIGNIN_RES;
				}
				//	res->res == eReturnCode::SIGNUP_FAIL ||
				//	res->res == eReturnCode::SIGNUP_ALREADY_IN_USE ||
				//	res->res == eReturnCode::SIGNUP_SUCCESS
				else
				{
					pid = Pktid::SIGNUP_RES;
				}

				PktRes pres = { res->usercode, "" }; // Signin, Signup�� token�� �������� ����. -> �����ϰ� �ٲܱ�..?

				PktHeader stPktHeader = { SEND_DATASIZE, pid };
				SendPacketData* packetData = new SendPacketData(res->clientIndex, stPktHeader, pres);

				PushSendPacket(packetData);

				delete res;
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	}

	void PushSendPacket(SendPacketData* pData_)
	{
		IOCPServer::SendMsg(pData_);
		return;
	}

	// �����ð����� ����� �� �ִ� ��ū�� �����Ͽ� SendPacketData�������� ������ �� ��ȯ�Ѵ�.
	SendPacketData* MakeSendPacket(const unsigned short sessionIndex_, const long userid_)
	{
		PktHeader stPktHeader = { SEND_DATASIZE, Pktid::TOKEN_RES };
		PktRes stPktRes;

		time_t now = time(NULL);
		now += LIFE_OF_TOKEN_SEC;

		PktInfo stPktInfo = { userid_, now };

		encrypt(sessionIndex_, stPktInfo, stPktRes);

		SendPacketData* ret = new SendPacketData(sessionIndex_, stPktHeader, stPktRes);

		return ret;
	}

	// PktInfo�� ������ ��ȣȭ�Ͽ� PktRes�� char�������� ��´�.
	
	void encrypt(const unsigned short sessionIndex_, const PktInfo& stPktInfo_, PktRes& out_)
	{
		unsigned char text[TOKEN_SIZE + 16] = { 0 }; // AES Padding ���
		CopyMemory(text, &stPktInfo_, TOKEN_SIZE - 1);

		int ciphertext_len = encryptAES(text, TOKEN_SIZE - 1, AES_PRIVATE_KEY, AES_PRIVATE_IV, text);

		if (ciphertext_len > TOKEN_SIZE + 16)
		{
			std::cerr << "AuthServer::encrypt : Seg fault.\n";
			return;
		}

		// base64 ���ڵ��� �߰�����. (��ȣȭ �� ���� NULL�� �����ϰ��� �߰�.)
		char* tmp = nullptr;
		int numbytes = TOKEN_SIZE + 16;
		int iret = base64_encode((char*)text, numbytes, tmp);
		if (iret != numbytes)
		{
			std::cerr << "AuthServer::encrypt : base64 encoding error.\n";
			delete[] tmp;
			return;
		}

		std::string str(tmp);
		out_.token = str;

		delete[] tmp;
		return;
	}

	bool mIsRun = false;
	std::thread mProcessThread;
	std::thread mDBThread;
	std::mutex mRecvLock;

	RedisManager mRedis;
	DataBase mDB;
};