#include "GameServer.hpp"
#include <conio.h>

/*
* TCP, UDP IOCP Lib.
* 
* 작업기간 2025-10-13 ~ 2025-10-31
* 작성일 2025-10-31
* 작성자 psyc
* 
*  !! how to use !!
* 서버에서 처리하고자 하는 요청을 추가하는 경우
* 1. Job 클래스를 상속한 파생클래스 작성.
*  - Parse : JobFactory에서 헤더부분을 제외하고 페이로드를 std::string_view& 형식으로 전달해줌. 해당 내용으로 파라미터 구조체를 채우면 됨.
*  - Execute : 실제 처리 + 처리한 결과를 담을 PacketData* 객체를 패킷풀로부터(싱글턴) 받아오기 + 처리한 결과를 PacketData*에 작성하기.
*				처리 결과도 구조체를 하나 작성해 초기화한 뒤 PacketData::Init함수로 직렬화하면 편함. (템플릿 함수 참고. 헤더부분은 앞선 2개의 파라미터로 자동 작성되며, T&로 들어온 데이터는 페이로드.)
*  - SendRes : Execute 단계에서 작성한 메시지를 송신큐에 등록하면 된다. m_stDI->sendFunc을 통해 수행.
*				송신함수에서 사용할 송신프로토콜을 고를 수 있다. SendProtocol::TCP, SendProtocol::UDP를 파라미터로 넣어 수행하면 된다.
* 2. 해당 파생클래스를 JobFactory에 Register 함수로 등록
* 3. 해당 파생클래스를 Jobs = std::variant < > 구문 내에 추가. (메모리풀 블록 크기 추정 목적.)
* 
* 작업을 처리하는데 필요한 객체를 의존성 주입해야하는 경우
* 1. Job.hpp에 DIStruct 클래스가 선언되어 있다. 해당 클래스를 수정
* 2. JobFactory의 Init함수에서 DIStruct를 초기화한다. 이후 JobFactory에서 알아서 의존성을 주입해줄테니 생성자를 수정.
* 3. JobFactory::Init은 GameServer::Start에서 1회 수행된다. 해당 부분에서 적절하게 파라미터를 변경해주면 된다.
* 
* UDP 로직
* 일단 기본적으로 TCP요청으로 ReqTokenJob을 생성해두었다.
* 해당 TCP요청을 통해 클라이언트는 자신의 TCP연결에 맞는 토큰을 발급받을 수 있고,
* 해당 토큰을 UDP요청 헤더에 넣어 해당 요청이 자신임을 식별할 수 있도록 한다.
* 서버는 토큰식별이 된 요청에 대해 UDP IP,Port를 TCP연결객체에 갱신하여 이후 UDP로 응답을 전송해야하는 경우에 사용한다.
* 해당 토큰은 빠른발급과 충돌방지, 최소한의 스푸핑방지를 위해 연결객체의 인덱스를 상위 4바이트, 현재시간기반의 변수를 하위 4바이트로 결합한 8바이트 정수로 사용한다.
* 연결객체의 인덱스로 구분할 수 있기 때문에 다른 연결과 토큰충돌하는 일이 없다.
* 
* RDB
* odbc를 통해 MSSQL 연동이 가능하게 해두었다. (DB_NAME과 DB_ID, DB_PW는 수정할 것)
* 연결핸들과 구문핸들은 임계영역 없이 수행할 수 있도록 TLS로 작성하였고,
* 해당 핸들 발급/해제는 작업을 처리하는 스레드풀의 워커스레드의 시작과 끝에 작성해두었다.
* 환경핸들은 싱글턴 인스턴스를 호출하는 순간 생성자에서 발급하고, 이후 프로그램이 종료할 때 소멸자에서 해제하게 해두었다.
* 
* IOCP Scatter/Gather
* TCP패킷에만 적용해두었다. 
* 송신버퍼큐는 원형큐의 형태로 구현, PacketData* 객체를 담아두었다가,
* 송신이 발생할 때마다 WSABUF[]의 형태로 반환. 해당 WSABUF배열을 Scatter/Gather형태로 송신.
* 이후 커널버퍼에 담았다는 완료통지가 오면 해당 바이트만큼 제거.
* 개별 패킷의 사이즈 이상이면 해당 패킷을 패킷풀에 반환하고,
* 해당 패킷의 일부만 전송되었다면 해당 패킷에서 해당 부분을 Pop.
* 
* ThreadPool
* 시간기반 우선순위큐와, 일반 락프리큐 기반으로 각각 작성해두었다.
* 일반 락프리큐는 락이 필요 없기 때문에 std::atomic_wait를 활용하여 스레드를 깨울 수 있도록 작성.
* 사용할지는 모르겠지만 실시간으로 스레드의 갯수를 조정할 수 있도록 작성하였다.
* 시간기반 우선순위큐는 top에 있는 작업의 수행시간이 되었을 때 스레드를 깨울 수 있도록 std::condition_variable을 사용.
* 어차피 std::condition_varible을 사용하는 경우 락을 사용하게 되므로 Concurrency::concurrent_priority_queue는 사용하지 않고 일반 std::priority_queue를 사용.
* 
* MemoryPool
* JobFactory에서 사용.
* 다양한 크기를 갖는 객체들을 할당할 수 있는 메모리블록을 ::operator new를 통해 미리 할당해두었다가,
* 동적할당이 필요할 때, 해당 풀에서 메모리블록을 발급받은 후,
* 메모리블록에 Placement New로 객체를 생성한다.
* 이 과정에서 OS에 동적할당을 요청하는 동작이 제거되었으므로 해당 부분에서 오버헤드 감소 효과를 볼 수 있음.
* 사용이 끝난 이후 재사용을 위해 소멸자를 호출한 뒤 메모리블록을 풀에 반환한다. 이 부분도 OS요청이 없어 오버헤드 감소.
* MemoryPool이 소멸할 때 ::operator delete를 통해 메모리블록을 해제한다.
* 
* SpinLock
* lock, try_lock, unlock의 기능을 수행할 수 있다.
* try_lock은 1회 락습득을 시도한다. 반환값은 락 습득 여부다.
* lock은 busy하게 CPU를 점유하며 try_lock을 반복하고, 락을 습득하면 반환된다.
* lock은 반복수행하기 때문에 compare_exchange_weak를 통해 수행.
* try_lock은 1회 시도하기 때문에 compare_exchange_strong를 통해 수행. (weak를 쓰면 spurious failure를 방지할 수 없다.)
* unlock은 습득한 락을 해제한다. (단 이건 락의 보유 여부를 확인하지 않는다. 주의.)
* SpinLockGuard
* RAII를 적용하여 SpinLockGuard를 사용할 수도 있다. std::lock_guard처럼 사용할 수 있으며,
* try_lock 옵션을 적용하여 사용할 수도 있다. (이 경우 owns_lock을 조회할 것.)
* try_lock을 통해 생성한 SpinLockGuard는 소멸자에서 본인이 락을 가지고 있는지 확인하고 해제한다. (락습득에 실패한 경우는 해제하지 않는 셈.)
* try_lock으로 SpinLockGuard를 사용하는 예시는 GameServer::HandleReq에서 확인할 수 있다.
* RAII패턴으로 사용하되, 락 습득을 기다리지 않고 반환한 뒤 락 습득에 실패했다면 다른 스레드가 처리할 것이므로 작업 종료.
*/

int main()
{
	LogManager::SetPrintLevel(PrintLevel::PRINT_ALL);

	LOG_ERR("에러는 찍히는지 체크");
	LOG_DEBUG("디버그도 찍히는지 체크");

	GameServer server;
	server.Start(SERVER_PORT, UDP_PORT, MAX_CLIENT);

	char c = _getch();

	server.End();

	return 0;
}