#pragma once

#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

/*
* 전송/수신할 내용은 다음과 같다.
* 1. 로그인요청
* 2. 로그인응답 (토큰 전송)
* 3. 토큰재발행
* 4. 회원가입 요청
* 5. 회원가입 응답..? 2번으로 대체?
* 
* 헤더와 바디만 구현해두고 네트워크쪽에서 사이즈에 맞게 복사해주자.
* -> json 직렬화를 채택.
* 
* !!주의!!
* 패킷 구조를 바꿀경우 Json직렬화 코드를 수정해야 한다.
* 
* 
* 네트워크 패킷 가공 과정
* client -> auth -> client -> feat
* 
* **client -> auth
* 1. to json string
* 2. send (비대칭키가 아닌이상 암호화가 의미없을것같다..)
* ㄴ sniffing 이나 spoofing을 생각할 때,
* 모든 송수신쌍에 대해 비대칭키 키교환을 하지 않는이상 암호화는 큰 의미가 없을 것 같다.
* 만약 비대칭키 키교환을 한다면 나중에..
* 
* 1. recv
* 2. json string to struct
* 
* **auth -> client
* 1. token create (and push to redis, token:usercode)
* 2. AES encoding (only token)
* 3. Base64 encoding (only token)
* 4. to json string 
* 5. send
* 
* 1. recv
* 2. json string to struct (use token not decoded)
* 
* **client -> feat
* 1. to json string
* 2. AES encoding 
* 3. Base64 encoding 
* 
* 4. Send 
* 
* 1. Recv
* 
* 2. Base64 decoding
* 3. AES decoding
* 4. json string to struct
*/

enum class Pktid
{
	SIGNUP_REQ,
	SIGNIN_REQ,
	TOKEN_REQ,
	SIGNUP_RES,
	SIGNIN_RES,
	TOKEN_RES
};

struct PktHeader
{
	short TotalSize;
	Pktid pktid;
};

struct PktReq
{
	std::string id;
	std::string pw;
};

// 해당 정보를 CopyMemory를 통해 char배열로 옮긴 뒤, 암호화 후 PktRes에 담는다.
struct PktInfo
{
	long userid;
	unsigned long long time;
};

const long TOKEN_SIZE = sizeof(PktInfo) + 1;

struct PktRes
{
	long userid;
	std::string token;
};

struct PktSend
{
	short TotalSize;
	Pktid pktid;
	long userid;
	std::string token;
};

const unsigned long long SEND_DATASIZE = sizeof(PktHeader) + sizeof(PktRes);
const unsigned long long RECV_DATASIZE = sizeof(PktHeader) + sizeof(PktReq);

std::string SendDataToJsonString(const PktSend& pkt)
{
	rapidjson::Document doc;
	doc.SetObject();

	// short TotalSize;
	doc.AddMember("totalsize", (int)pkt.TotalSize, doc.GetAllocator());

	// Pktid pktid;
	switch (pkt.pktid)
	{
	case Pktid::SIGNIN_RES:
		doc.AddMember("pktid", "SIGNIN_RES", doc.GetAllocator());
		break;
	case Pktid::SIGNUP_RES:
		doc.AddMember("pktid", "SIGNUP_RES", doc.GetAllocator());
		break;
	case Pktid::TOKEN_RES:
		doc.AddMember("pktid", "TOKEN_RES", doc.GetAllocator());
		break;
	default:
		std::cerr << "Packet::SendDataToJsonString : Pktid Err\n";
	}

	// long userid;
	doc.AddMember("userid", (int)pkt.userid, doc.GetAllocator());

	// std::string token;
	doc.AddMember("token", rapidjson::Value().SetString(pkt.token.c_str(), static_cast<rapidjson::SizeType>(pkt.token.length())), doc.GetAllocator());

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	return buffer.GetString();
}

struct RecvPacketData
{
	PktHeader mPktHeader;
	PktReq mPktReq;
};

bool parseDataFromJson(const std::string& jsonString_, RecvPacketData& out_)
{
	rapidjson::Document doc;
	
	if (doc.Parse(jsonString_.c_str()).HasParseError())
	{
		std::cerr << "JSON Parsing Error\n";
		return false;
	}

	if (!doc.IsObject())
	{
		std::cerr << "Not JSON Obj.\n";
		return false;
	}

	if (doc.HasMember("totalsize") && doc["totalsize"].IsInt())
	{
		out_.mPktHeader.TotalSize = doc["totalsize"].GetInt();
	}
	else
	{
		std::cerr << "Missing or Invalid 'TotalSize' field\n";
		return false;
	}

	if (doc.HasMember("pktid") && doc["pktid"].IsString())
	{
		std::string str = doc["pktid"].GetString();

		if (!str.compare("SIGNIN_REQ"))
		{
			out_.mPktHeader.pktid = Pktid::SIGNIN_REQ;
		}
		else if (!str.compare("SIGNUP_REQ"))
		{
			out_.mPktHeader.pktid = Pktid::SIGNUP_REQ;
		}
		else if (!str.compare("TOKEN_REQ"))
		{
			out_.mPktHeader.pktid = Pktid::TOKEN_REQ;
		}
		else
		{
			std::cerr << "Not Defined pktid.\n";
			return false;
		}
	}
	else
	{
		std::cerr << "Missing or Invalid 'pktid' field\n";
		return false;
	}

	if (doc.HasMember("id") && doc["id"].IsString())
	{
		out_.mPktReq.id = doc["id"].GetString();
	}
	else
	{
		std::cerr << "Missing or Invalid 'id' field\n";
		return false;
	}

	if (doc.HasMember("pw") && doc["pw"].IsString())
	{
		out_.mPktReq.pw = doc["pw"].GetString();
	}
	else
	{
		std::cerr << "Missing or Invalid 'pw' field\n";
		return false;
	}

	return true;
}

class SendPacketData
{
public:
	const unsigned short SessionIndex;
	char* pPacketData;
	stOverlappedEx sendOverlapped;

	// for Redis parameter
	long usercode;
	char mToken[TOKEN_SIZE + 16];

	// 새로운 패킷을 생성.
	SendPacketData(const unsigned short sessionIndex_, PktHeader& stPktHeader_, PktRes& stPktRes_) : SessionIndex(sessionIndex_)
	{
		PktSend pkt;
		pkt.pktid = stPktHeader_.pktid;
		pkt.TotalSize = stPktHeader_.TotalSize;
		pkt.token = stPktRes_.token;
		pkt.userid = stPktRes_.userid;
		usercode = pkt.userid;

		std::string sendstr = SendDataToJsonString(pkt);

		pPacketData = new char[sendstr.size() + 1];
		strcpy_s(pPacketData, sendstr.size() + 1, sendstr.c_str());

		ZeroMemory(mToken, TOKEN_SIZE + 16);
		strcpy_s(mToken, TOKEN_SIZE+16, stPktRes_.token.c_str());
	}

	~SendPacketData()
	{
		delete pPacketData;
	}

	// overlapped 구조체를 초기화하여 바로 송신에 사용할 수 있도록 함.
	void SetOverlapped()
	{
		ZeroMemory(&sendOverlapped, sizeof(stOverlappedEx));

		sendOverlapped.m_wsaBuf.len = SEND_DATASIZE;
		sendOverlapped.m_wsaBuf.buf = pPacketData;

		sendOverlapped.m_eOperation = eIOOperation::SEND;
		sendOverlapped.m_userIndex = SessionIndex;
	}
};
