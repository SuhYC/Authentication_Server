/*
* 해당 클래스는 반드시 AuthToken을 발행하여 인증하는 데에만 사용하도록 한다.
* 
* 1. IOCP : Receive Data And Toss To AuthServer
* 2. AuthServer : ReqToken To AES
* 3. AES : MakeToken
* 4. AuthServer : Push Token To RedisManager
* 5. RedisManager : Push Token To Redis
* 6. AuthServer : Get Token which was processed to Redis
* 7. IOCP : Send Token To Client
*/

#pragma once

#include "Define.hpp"
#include "Packet.hpp"

#include <hiredis/hiredis.h>
#include <iostream>
#include <queue>
#include <mutex>

const bool REDIS_DEBUG = true;
const int REDIS_THREADS = 2;

const char* REDIS_IP = "127.0.0.1";
const int REDIS_PORT = 6379;

class RedisManager
{
public:
	void Init()
	{
		CreateThread();
	}

	void End()
	{
		DestroyThread();

		FlushQueue();
	}

	void RedisTaskThread()
	{
		redisContext* rc = redisConnect(REDIS_IP, REDIS_PORT);

		if (rc == NULL || rc->err)
		{
			if (rc)
			{
				std::cerr << "Redis 연결 실패: " << rc->errstr << "\n";
				redisFree(rc);
			}
			else
			{
				std::cerr << "RedisContext 할당 실패\n";
			}
			return;
		}

		if (REDIS_DEBUG)
		{
			std::lock_guard<std::mutex> guard(debugMutex);
			std::cout << "Redis 연결 성공...\n";
		}

		while (mIsRun)
		{
			SendPacketData* pkt = DequeReq();

			if (pkt != nullptr)
			{
				redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(rc, "SETEX %s %d %d", pkt->mToken, LIFE_OF_TOKEN_SEC, pkt->usercode));

				if (reply == nullptr)
				{
					std::cerr << "SETEX Command Failed\n";

					// 작업이 끝나지 않았으므로 다시 큐에 넣는다.
					PushReq(pkt);

					freeReplyObject(reply);
					continue;
				}

				if (REDIS_DEBUG)
				{
					std::lock_guard<std::mutex> guard(debugMutex);
					std::cout << "SETEX: " << reply->str << "\n";
				}

				freeReplyObject(reply);
				PushResult(pkt);
				continue;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		redisFree(rc);

		return;
	}

	// 외부시스템에서 InQueue로 요청을 Push
	void PushReq(SendPacketData* data_)
	{
		// 서버 중단 시 큐에 담지 않고 할당해제.
		if (!mIsRun)
		{
			delete data_;
			return;
		}

		std::lock_guard<std::mutex> guard(mInLock);

		InQueue.push(data_);

		return;
	}

	// 외부시스템에서 Redis작업이 끝난 데이터를 OutQueue로부터 Pop
	SendPacketData* GetRes()
	{
		std::lock_guard<std::mutex> guard(mOutLock);

		if (OutQueue.empty())
		{
			return nullptr;
		}

		SendPacketData* ret = OutQueue.front();
		OutQueue.pop();

		return ret;
	}

private:
	
	void CreateThread()
	{
		mIsRun = true;

		for (int i = 0; i < REDIS_THREADS; i++)
		{
			TaskThreads.emplace_back([this]() { RedisTaskThread(); });
		}

		return;
	}

	void DestroyThread()
	{
		mIsRun = false;

		for (auto& i : TaskThreads)
		{
			if (i.joinable())
			{
				i.join();
			}
		}

		return;
	}

	SendPacketData* DequeReq()
	{
		std::lock_guard<std::mutex> guard(mInLock);

		if (InQueue.empty())
		{
			return nullptr;
		}

		SendPacketData* ret = InQueue.front();
		InQueue.pop();

		return ret;
	}

	void PushResult(SendPacketData* data_)
	{
		std::lock_guard<std::mutex> guard(mOutLock);

		OutQueue.push(data_);

		return;
	}

	// 서버 중단 시 큐 비움.
	void FlushQueue()
	{
		std::lock_guard<std::mutex> guard(mOutLock);

		while (!OutQueue.empty())
		{
			delete OutQueue.front();
			OutQueue.pop();
		}

		return;
	}

	SendPacketDataQueue InQueue;
	SendPacketDataQueue OutQueue;
	std::mutex mInLock;
	std::mutex mOutLock;
	std::vector<std::thread> TaskThreads;

	bool mIsRun = false;
};