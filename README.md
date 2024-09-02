# Authentication_Server
미완성.

cpp project

## skills
IOCP
Redis - hiredis
MSSQL - odbc
AES - openssl
Base64

## As-Is
현재에 와서는 인증 기능을 Oauth에 맡기는 형태가 일반적이다. </br>
하지만 한번쯤 인증서버가 갖춰야할 기본적인 형태를 생각하며 만들어볼만 하다고 판단하여 시작. </br>
인증서버 자체가 감당할 부하는 그렇게 크지 않다고 생각하지만, 만약 다중서버로 구현해야한다면? 에서 출발하여 다중서버 구조를 고려해보기로함.</br>

## 구현
Auth Server는 C++의 IOCP를 활용하여 만든다. </br>
유저의 계정정보는 MSSQL - SQLServer를 사용하여 저장한다. 이는 Auth Server에서 odbc를 사용하여 연결한다. </br>
로그인되어있는 유저의 정보는 간단하게 unordered map을 활용해 만드는 경우가 많지만, 다중서버를 고려한다고 했으니 SQLServer에 어떤 인증서버가 인증중인지 기록하기로 한다.
인증서버가 다운되는 경우를 대비하여 실행 시 해당 인증서버의 번호로 인증 중인 유저의 로그인정보를 초기화한다.
SQLServer와 통신하여 계정정보가 확인된 유저는 토큰을 발급하며 해당 토큰은 SharedDB에 저장하여 기능서버에서도 확인할 수 있도록 한다.
SharedDB로는 Redis를 채택한다.

## To-Do
Unity로 클라이언트 제작
기능서버 제작
오류 개선
