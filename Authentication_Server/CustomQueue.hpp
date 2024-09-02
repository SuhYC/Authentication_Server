#pragma once

#include "Packet.hpp"
#include "Define.hpp"

class SendPacketDataQueue
{
public:

	SendPacketDataQueue()
	{
		Head = nullptr;
		Tail = nullptr;
		FreeList = nullptr;
		mSize = 0;
	}

	~SendPacketDataQueue()
	{
		while (Head != nullptr)
		{
			Node* tmp = Head;
			Head = Head->next;
			delete tmp;
		}

		while (FreeList != nullptr)
		{
			Node* tmp = FreeList;
			FreeList = FreeList->next;
			delete tmp;
		}
	}

	void push(SendPacketData* pData_)
	{
		if (empty())
		{
			Head = getNewNode(pData_);
			Tail = Head;
			mSize++;
		}
		else
		{
			Tail->next = getNewNode(pData_);
			Tail = Tail->next;
			mSize++;
		}
	}

	void pop()
	{
		Node* tmp = Head;
		Head = Head->next;
		discard(tmp);
		mSize--;
	}

	SendPacketData* front() const { return Head->data; }

	bool empty() const { return mSize == 0; }
	size_t size() const { return mSize; }

private:
	class Node
	{
	public:
		Node(SendPacketData* pData_) : data(pData_) { next = nullptr; }
		~Node() {}

		// pop�� ���Ǹ� data�� free���� �ʴ´�. ������ ��
		// free���� �ʴ� ������ front�� ���� �����͸� ������ ������ �޸� ��ȯ�� �ذ��ؾ��ϱ� ����.
		void release()
		{
			data = nullptr;
			next = nullptr;
		}
		
		void set(SendPacketData* pData_)
		{
			data = pData_;
		}

		Node* next;
		SendPacketData* data;
	};

	// �������� ��带 free���� �ʰ� freeList�� �ִ´�.
	// ��üǮ�� Ȱ���Ͽ� �޸� �Ҵ� ������带 ���̱� ����
	void discard(Node* pNode_)
	{
		pNode_->release();
		if (FreeList == nullptr)
		{
			FreeList = pNode_;
		}
		else
		{
			pNode_->next = FreeList;
			FreeList = pNode_;
		}
	}

	// FreeList���� ��带 �ϳ� �����´�.
	// ���� ���� ��尡 ���ٸ� �޸𸮸� �Ҵ��Ѵ�.
	Node* getNewNode(SendPacketData* pData_)
	{
		if (FreeList == nullptr)
		{
			return new Node(pData_);
		}
		else
		{
			Node* tmp = FreeList;
			FreeList = FreeList->next;
			tmp->set(pData_);

			return tmp;
		}
	}

	Node* Head;
	Node* Tail;
	Node* FreeList;
	size_t mSize;
};

class DBRequestQueue
{
public:

	DBRequestQueue()
	{
		Head = nullptr;
		Tail = nullptr;
		FreeList = nullptr;
		mSize = 0;
	}

	~DBRequestQueue()
	{
		while (Head != nullptr)
		{
			Node* tmp = Head;
			Head = Head->next;
			delete tmp;
		}

		while (FreeList != nullptr)
		{
			Node* tmp = FreeList;
			FreeList = FreeList->next;
			delete tmp;
		}
	}

	void push(DBRequest* pData_)
	{
		if (empty())
		{
			Head = getNewNode(pData_);
			Tail = Head;
			mSize++;
		}
		else
		{
			Tail->next = getNewNode(pData_);
			Tail = Tail->next;
			mSize++;
		}
	}

	void pop()
	{
		Node* tmp = Head;
		Head = Head->next;
		discard(tmp);
		mSize--;
	}

	DBRequest* front() const { return Head->data; }

	bool empty() const { return mSize == 0; }
	size_t size() const { return mSize; }

private:
	class Node
	{
	public:
		Node(DBRequest* pData_) : data(pData_) { next = nullptr; }
		~Node() {}

		// pop�� ���Ǹ� data�� free���� �ʴ´�. ������ ��
		// free���� �ʴ� ������ front�� ���� �����͸� ������ ������ �޸� ��ȯ�� �ذ��ؾ��ϱ� ����.
		void release()
		{
			data = nullptr;
			next = nullptr;
		}

		void set(DBRequest* pData_)
		{
			data = pData_;
		}

		Node* next;
		DBRequest* data;
	};

	// �������� ��带 free���� �ʰ� freeList�� �ִ´�.
	// ��üǮ�� Ȱ���Ͽ� �޸� �Ҵ� ������带 ���̱� ����
	void discard(Node* pNode_)
	{
		pNode_->release();
		if (FreeList == nullptr)
		{
			FreeList = pNode_;
		}
		else
		{
			pNode_->next = FreeList;
			FreeList = pNode_;
		}
	}

	// FreeList���� ��带 �ϳ� �����´�.
	// ���� ���� ��尡 ���ٸ� �޸𸮸� �Ҵ��Ѵ�.
	Node* getNewNode(DBRequest* pData_)
	{
		if (FreeList == nullptr)
		{
			return new Node(pData_);
		}
		else
		{
			Node* tmp = FreeList;
			FreeList = FreeList->next;
			tmp->set(pData_);

			return tmp;
		}
	}

	Node* Head;
	Node* Tail;
	Node* FreeList;
	size_t mSize;
};

class DBResultQueue
{
public:

	DBResultQueue()
	{
		Head = nullptr;
		Tail = nullptr;
		FreeList = nullptr;
		mSize = 0;
	}

	~DBResultQueue()
	{
		while (Head != nullptr)
		{
			Node* tmp = Head;
			Head = Head->next;
			delete tmp;
		}

		while (FreeList != nullptr)
		{
			Node* tmp = FreeList;
			FreeList = FreeList->next;
			delete tmp;
		}
	}

	void push(DBResult* pData_)
	{
		if (empty())
		{
			Head = getNewNode(pData_);
			Tail = Head;
			mSize++;
		}
		else
		{
			Tail->next = getNewNode(pData_);
			Tail = Tail->next;
			mSize++;
		}
	}

	void pop()
	{
		Node* tmp = Head;
		Head = Head->next;
		discard(tmp);
		mSize--;
	}

	DBResult* front() const { return Head->data; }

	bool empty() const { return mSize == 0; }
	size_t size() const { return mSize; }

private:
	class Node
	{
	public:
		Node(DBResult* pData_) : data(pData_) { next = nullptr; }
		~Node() {}

		// pop�� ���Ǹ� data�� free���� �ʴ´�. ������ ��
		// free���� �ʴ� ������ front�� ���� �����͸� ������ ������ �޸� ��ȯ�� �ذ��ؾ��ϱ� ����.
		void release()
		{
			data = nullptr;
			next = nullptr;
		}

		void set(DBResult* pData_)
		{
			data = pData_;
		}

		Node* next;
		DBResult* data;
	};

	// �������� ��带 free���� �ʰ� freeList�� �ִ´�.
	// ��üǮ�� Ȱ���Ͽ� �޸� �Ҵ� ������带 ���̱� ����
	void discard(Node* pNode_)
	{
		pNode_->release();
		if (FreeList == nullptr)
		{
			FreeList = pNode_;
		}
		else
		{
			pNode_->next = FreeList;
			FreeList = pNode_;
		}
	}

	// FreeList���� ��带 �ϳ� �����´�.
	// ���� ���� ��尡 ���ٸ� �޸𸮸� �Ҵ��Ѵ�.
	Node* getNewNode(DBResult* pData_)
	{
		if (FreeList == nullptr)
		{
			return new Node(pData_);
		}
		else
		{
			Node* tmp = FreeList;
			FreeList = FreeList->next;
			tmp->set(pData_);

			return tmp;
		}
	}

	Node* Head;
	Node* Tail;
	Node* FreeList;
	size_t mSize;
};