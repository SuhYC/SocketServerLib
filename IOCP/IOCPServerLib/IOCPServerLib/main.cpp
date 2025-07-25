#pragma once

/********************************************************************************************************
* Project : IOCP GameServer Framework Project
* Author : 서예찬 (psyc / psyc0702@naver.com)
*			Github : https://github.com/SuhYC
* Created : 2025-07-07
* Last Update : 2025-07-09
* Description : IOCP 게임서버 프레임워크 구현
* 
* IOCP 관련으로 더이상 서버코어 작성에 시간을 투자하지 않기 위해 진행.
* 이후 진행할 프로젝트는 다른 기술의 서버거나, 혹은 해당 프레임워크를 활용한 컨텐츠 개발에 집중.
* 
* Notes : 
*  - IOCP 기반 비동기 송수신 처리
*  - WorkerThread는 송수신에 대한 처리만 수행. 수신정보는 Job객체로 파싱하여 Enqueue.
*  - JobThread는 Job객체를 가져와 병렬처리.
*  - 송신버퍼가 가득 차 더 이상 송신할 수 없으면 다시 Re-Enqueue. 이후 송신버퍼가 비워지면 다시 송신 시도
* 
*  - UserManager
*	연결된 유저의 유저정보를 담을 클래스.
*	상속하여 다른 형태로 확장해도 괜찮음.
*	User의 파생클래스를 관리하는 UserManager의 파생클래스 형태도 괜찮음.
* 
*  - ZoneManager
*	플레이어가 구분된 영역에서 플레이할 수 있도록 구별하는 Zone을 관리하는 클래스.
*	상속하여 다른 형태로 확장해도 괜찮음.
*	Zone의 파생클래스를 관리하는 ZoneManager의 파생클래스 형태도 괜찮음.
* 
*  - UserManager와 ZoneManager를 템플릿 클래스가 아닌 상속을 통해 작성한 이유
*	Job객체는 기본적으로 처리를 위해 UserManager와 ZoneManager가 필요함.
*	의존객체로서 UserManager와 ZoneManager를 가지고 있어야하는데 이게 템플릿으로 작성하면 구조가 복잡해지게 됨.
*	다형성 처리를 위해 상속으로 구현.
*	상속될 여지가 있으므로 GetUser는 final 제한자를 걸었으나 CreateUser와 Init는 오버라이딩이 가능하게 만들어 놨음.
* 
*  - MemoryPool
*	1. JobMemoryPool
*		Job객체는 수신한 요청 하나마다 할당되어야함. 할당과 해제의 빈도가 너무 잦기 때문에 메모리풀을 활용.
*		Jobs라는 union을 설정해놨음. union의 크기는 내부에 저장할 수 있는 데이터 중 가장 큰 크기를 갖기 때문에
*		union의 size를 조사하여 Job 파생클래스들의 크기 중 가장 큰 크기를 기반으로 메모리블록을 생성.
*		단, 메모리풀이긴 하나 Factory패턴을 사용해 외부에서 생성자 호출을 할 필요는 없게 설계.
*	2. PacketPool
*		Packet객체는 마찬가지로 유저의 요청마다 발생한 응답을 담아야하기 때문에 할당과 해제의 빈도가 너무 잦음.
*		다만 Packet객체는 내부 버퍼를 가지고 있어 메시지마다 다르게 생성할 필요 없이 Packet객체만 생성하면 되기 때문에
*		오브젝트 풀로 관리.
* 
*	- SlideBuffer
*	송신 및 수신버퍼로 활용하기 위해 설계.
*	RingBuffer와의 차이점은 추상적으로 원형설계된 RingBuffer와는 달리
*	사용이 끝난 데이터는 제거하고 사용이 끝나지 않은 데이터를 앞으로 당기는 형태이기 때문에
*	버퍼의 양 끝단 처리를 할 필요가 없으며 읽을 때는 언제나 버퍼의 처음부분을 읽으면 된다는 장점이 있음.
*	다만 write 동작이 비동기로 이루어지는 경우 데이터를 제거하기 어렵다는 단점이 있음.
*	(수신 예약으로 해당 버퍼를 직접 사용하는 것은 불가능에 가까움. 다른 버퍼에 수신하여 옮겨담아야함)
* 
*	- Job
*	WorkerThread에서 수신, 파싱 후 처리까지 하는 구조에서 처리부분을 분리하기 위해 만든 객체.
*	수행해야할 동작의 파라미터 정보를 담고 있으며 처리에 필요한 의존객체를 static 필드로 갖는다.
*
*	- JobFactory
*	수신한 메시지를 파싱하여 요청타입에 맞는 Job파생클래스를 생성하고, 해당 파라미터정보를 초기화하는 클래스.
*	Factory패턴을 사용하며 UserManager와 ZoneManager 같은 의존 객체를 의존성주입으로 Job 파생 객체에 담아 반환한다.
* 
*	- SpinLock
*	std::mutex와 달리 busy-wait로 작동하는 락.
*	std::atomic을 활용해 cas연산을 사용하여 구현.
*	중복호출로 인한 데드락은 방지하지 않았다. 사용에 유의할 것.
*	RAII패턴으로 SpinLockGuard가 구현되어있다. std::lock_guard처럼 스코프에 선언하여 사용할 수 있다.
* 
*	- LogManager
*	표준출력에 로그를 출력하는 클래스이다.
*	내부에 락이 있어 출력 중에 꼬이는 일을 방지.
*	PrintLevel을 설정하여 원하는 로그만 찍을 수 있다.
*	1. PrintLevel::PRINT_ALL은 모든 Criticality의 로그를 출력한다.
*	2. PrintLevel::PRINT_ERR는 Criticality::ERR의 로그만 출력한다.
*	3. PrintLevel::NO_PRINT는 모든 종류의 로그를 무시한다.
*	함수시그니처와 Criticality enum을 일일히 적기 번거롭기 때문에 Define 헤더에 매크로함수를 작성해 두었다.
*	LOG_ERR는 Criticality::ERR로 함수시그니처와 함께 출력한다.
*	LOG_DEBUG는 Criticality::DEBUG로 함수시그니처와 함께 출력한다.
*	
*	- DEFAULT_CATCH() 매크로
*	Define 헤더에 선언되어 있다.
*	try문 이후 catch문 열거가 끝난 후에 사용하면 된다.
*	std::exception& 타입 예외와 catch(...)의 모든 예외를 받아 LOG_ERR로 출력한다.
*	단, 사용할 부분의 반환타입이 어떻게 될 지 모르므로 catch문 내에서 return하지 않고 진행한다. 주의.
* 
*	!! How to Use !!
*	대부분의 환경변수는 Define 헤더에 작성되어있다. 필요 시 수정
*
*	새로운 작업요청을 생성할 경우
*	1. Define 헤더에서 ReqType에 새로운 코드를 LAST 바로 위에 작성하고, LAST = 부분에 새로운 코드를 입력할 것.
*	2. 요청에 쓰일 파라미터를 POD 구조체 형태로 생성. (NetworkMessage 헤더에 예시 있음)
*	3. 해당 파라미터를 갖는 Job 파생객체를 생성
*	4. JobMemoryPool 헤더에 있는 Jobs 유니온에 해당 파생클래스 추가
*	5. JobFactory::Init 함수에서 요청코드와 추가된 파생클래스로 Register함수 호출
* 
*	GameServer::OnReceive에서 GetReqMsg, Parsing, Queueing, PopReqMsg가 락없이 수행되는 이유는 모든 수신에대한 처리가 끝나고 나서야 BindRecv를 다시 걸기 때문에
*	동시에 여러 수신완료가 도착하지 않기 때문.
********************************************************************************************************/


#include "LogManager.hpp"
#include "Define.hpp"
#include "GameServer.hpp"
#include <conio.h>

int main()
{
	LogManager::SetPrintLevel(PrintLevel::PRINT_ALL);
	GameServer server(SERVER_PORT, MAX_CLIENT);

	server.Run();

	int a = _getch();

	server.End();

	return 0;
}