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

		// pop에 사용되며 data를 free하지 않는다. 주의할 것
		// free하지 않는 이유는 front를 통해 데이터를 가져간 측에서 메모리 반환을 해결해야하기 때문.
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

	// 버려지는 노드를 free하지 않고 freeList에 넣는다.
	// 객체풀을 활용하여 메모리 할당 오버헤드를 줄이기 위함
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

	// FreeList에서 노드를 하나 가져온다.
	// 만약 여분 노드가 없다면 메모리를 할당한다.
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

		// pop에 사용되며 data를 free하지 않는다. 주의할 것
		// free하지 않는 이유는 front를 통해 데이터를 가져간 측에서 메모리 반환을 해결해야하기 때문.
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

	// 버려지는 노드를 free하지 않고 freeList에 넣는다.
	// 객체풀을 활용하여 메모리 할당 오버헤드를 줄이기 위함
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

	// FreeList에서 노드를 하나 가져온다.
	// 만약 여분 노드가 없다면 메모리를 할당한다.
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

		// pop에 사용되며 data를 free하지 않는다. 주의할 것
		// free하지 않는 이유는 front를 통해 데이터를 가져간 측에서 메모리 반환을 해결해야하기 때문.
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

	// 버려지는 노드를 free하지 않고 freeList에 넣는다.
	// 객체풀을 활용하여 메모리 할당 오버헤드를 줄이기 위함
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

	// FreeList에서 노드를 하나 가져온다.
	// 만약 여분 노드가 없다면 메모리를 할당한다.
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