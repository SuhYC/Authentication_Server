#pragma once

#include "AuthServer.hpp"
#include <conio.h>

const int PORTNUM = 11250;
const int MAXCLIENT = 100;

int main()
{
	AuthServer server;
	server.Init(PORTNUM);

	server.Run(MAXCLIENT);

	_getch();

	server.End();


	return 0;
}

/* To Do List
* 1. IOCP를 사용해 클라이언트와 연결, 통신 기능
*  - 데이터 전송 수신 기능만 있어도 된다.
*  - 패킷을 구조체 형태로 만들고 헤더 기능도 추가한다.
* 2. Auth Server Core에 상속하여 관리
*  - IOCP에서 수신한 메시지를 용도에 따라 가공만 하여 큐에 담는다.
*  - 큐에서 데이터를 뽑아 DB요청하는 스레드를 만든다. 결과를 큐에 담는다.
*  - 결과를 큐에서 뽑아 다시 IOCP 전송한다.
* 3. DB에 연결하여 인증기능 활성화
*  - ID, PW, UserCode, LastAuthServer를 저장할 수 있도록 한다. (LastAuthServer : 서버클러스터로의 확장을 염두.)
*  - LastAuthServer에 null이 있는 경우 로그인이 가능하다. (0이나 -1으로 대체 가능)
*  - ID와 PW가 일치하며 LastAuthServer가 비어있는 경우 UserCode와 유효기간을 암호화하여 AToken을 발행한다. : 구조체를 char 메모리에 복사한 후 암호화 복호화
*  - Auth Server Core에서 세션에 UserCode와 AToken 발급시간을 저장하고 AToken을 클라이언트에게 전송한다.
*  - Auth Server Core에서는 발급시간으로부터 경과된 시간에 따라 다시 AToken을 발급할 것을 요청한다 (요청 없이 새로운 토큰을 발행해줘도 됨.)
*  - Auth Server Core에서는 만약 세션이 종료되면 LastAuthServer를 초기화할 것을 요청한다.
* 
* 4. Auth Manage Server를 통한 서버클러스터 구성 (여기는 할지 모르겠다. ㅠ)
*  - Auth Manage Server를 만들고 각 Auth Server가 몇번인지 기록한다.
*  - 특정 유저 인증을 마지막으로 인가해준 Server를 기록한다. (UserData의 마지막 컬럼. LastAuthServer)
*  - 만약 Auth Server가 다운되어 가지고 있던 세션정보를 확인할 수 없을 때
*  - 해당 컬럼으로 조회하여 다운된 서버가 인가했던 LastAuthServer를 모두 초기화하여 다시 접속할 수 있도록 한다.
* 
* 일단 확장성을 위해 병렬연결 방식으로 수행한다. 1개의 Auth Server로도 충분할 것 같긴 하지만, 혹시 모르니.. (+서버 다운에 대한 내성은 클러스터 형식이 좋긴 하니까?)
* Auth Server는 토큰만 발행해준다. 기능서버에서 Token을 해독하고 인가해준다. (즉, Feat Server에서는 Auth Server에 확인하는 형식이 아닌, 복호화를 진행하고 적절한 데이터가 왔다면 정상 처리한다.)
* 
* 이 때, 한 가지 더 고려해야할 사항이 있다.
* 만약 토큰이 탈취당한다면, 해커가 유저 대신 토큰을 사용할 수 있게 된다.
* 짧은 시간 유지되는 토큰을 발행하고, 해당 토큰은 어떤목적으로 쓰이는 지 또한 포함한다.
* (Web Server의 Refresh Token은 세션으로 대체하며, Access Token만 발행하는 셈.)
* 
* -> redis로 옮기자!
* redis를 shared db로 사용한다. (in memory db를 사용할 것이기 때문에 속도는 적게 고려해도 될것 같다.)
* 
* 굳이 auth server에서 관리 안하고 redis를 사용하는 이유
* auth server를 기획할 때 최대한 병렬성을 유지할 수 있게 다중서버를 기반으로 기획했기 때문에
* map을 이용해 토큰을 조사하더라도 여러개의 auth server를 동기화 할 수 없기 때문이다.
* 그러므로 동기화를 위한 redis db를 이용하고, auth server 에서는 토큰을 발급하고 client에 전송과 동시에 redis db에 저장한다.
* 
* feat server는 redis db에 토큰이 있는지 확인하고 동시에 적절한 토큰인지 확인한다.
* redis db에도 있으면서 적절한 토큰인 경우만 명령을 수용하여 처리한다.
* 
* 
* 중복로그인 확인 : AccountDB에 저장
* 토큰 발급 : Auth Server에서 발급 + AES 암호화 + Redis에 저장
* 토큰 확인 : Feat Server에서 Redis 조회 + AES 복호화
* 
*							  ┌───┐
*					  ┌───┤Redis ├─┐
*					  │	  └───┘  │
* ┌───┐      ┌─┴┐  ┌─────┐│
* │Client├───┤Auth├─┤Account DB││
* └─┬─┘      └──┘  └─────┘│
*     │		  ┌──┐				  │
*	  └─────┤Feat├────────┘
*				  └──┘
* 
* client -req token-> auth
* 
* auth -res token-> client
* auth -setex token->Redis
* 
* client -req act-> feat
* 
* feat -get token-> Redis
* feat -res result-> client
* 
* !!! redis에 이미 유효기간 처리가 가능하도록 만들어져 있다 !!!
* 토큰에서 시간정보는 빼도 될듯..
*/

