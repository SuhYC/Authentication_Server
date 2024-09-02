#pragma once

#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

/*
* ����/������ ������ ������ ����.
* 1. �α��ο�û
* 2. �α������� (��ū ����)
* 3. ��ū�����
* 4. ȸ������ ��û
* 5. ȸ������ ����..? 2������ ��ü?
* 
* ����� �ٵ� �����صΰ� ��Ʈ��ũ�ʿ��� ����� �°� ����������.
* -> json ����ȭ�� ä��.
* 
* !!����!!
* ��Ŷ ������ �ٲܰ�� Json����ȭ �ڵ带 �����ؾ� �Ѵ�.
* 
* 
* ��Ʈ��ũ ��Ŷ ���� ����
* client -> auth -> client -> feat
* 
* **client -> auth
* 1. to json string
* 2. send (���ĪŰ�� �ƴ��̻� ��ȣȭ�� �ǹ̾����Ͱ���..)
* �� sniffing �̳� spoofing�� ������ ��,
* ��� �ۼ��Žֿ� ���� ���ĪŰ Ű��ȯ�� ���� �ʴ��̻� ��ȣȭ�� ū �ǹ̰� ���� �� ����.
* ���� ���ĪŰ Ű��ȯ�� �Ѵٸ� ���߿�..
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

// �ش� ������ CopyMemory�� ���� char�迭�� �ű� ��, ��ȣȭ �� PktRes�� ��´�.
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

	// ���ο� ��Ŷ�� ����.
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

	// overlapped ����ü�� �ʱ�ȭ�Ͽ� �ٷ� �۽ſ� ����� �� �ֵ��� ��.
	void SetOverlapped()
	{
		ZeroMemory(&sendOverlapped, sizeof(stOverlappedEx));

		sendOverlapped.m_wsaBuf.len = SEND_DATASIZE;
		sendOverlapped.m_wsaBuf.buf = pPacketData;

		sendOverlapped.m_eOperation = eIOOperation::SEND;
		sendOverlapped.m_userIndex = SessionIndex;
	}
};
