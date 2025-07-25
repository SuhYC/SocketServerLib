#pragma once

#include <stdint.h>
#include <exception>
#include <iostream>
#include <string.h>

/// <summary>
/// lock-free���� �ʽ��ϴ�. ��ȣ���� �ʿ�.
/// </summary>
class SlideBuffer final
{
public:
	SlideBuffer();
	~SlideBuffer();

	/// <summary>
	/// �����̵���ۿ� ��� �޽����� ���ϴ�.
	/// �޸𸮴� ������� �ʽ��ϴ�.
	/// </summary>
	/// <returns></returns>
	void Clear();

	/// <summary>
	/// �����̵���ۿ� �޸𸮸� �Ҵ��մϴ�.
	/// ������ �Ҵ�� �޸𸮰� �־��ٸ� ���� �� �Ҵ��ϸ�
	/// ���� �޸𸮿� �ۼ��ߴ� �����ʹ� �������� �ʽ��ϴ�.
	/// </summary>
	/// <param name="capacity_">�ű� �Ҵ��� �޸��� ũ��</param>
	/// <returns>���Ҵ� ���� ����</returns>
	bool Init(const uint32_t capacity_);

	/// <summary>
	/// �����̵���ۿ� �����͸� �����մϴ�.
	/// ������ ���� �������� ū �����͸� �����ϸ� �����մϴ�.
	/// �����ͷ� nullptr�� �ѱ�� �����մϴ�.
	/// </summary>
	/// <param name="in_">������ �������� ������</param>
	/// <param name="size_">������ �������� ũ��</param>
	/// <returns>���� ���� ����</returns>
	bool Enqueue(const char* in_, const uint32_t size_);

	/// <summary>
	/// �����̵���۷κ��� �����͸� �����մϴ�.
	/// ��¹��۷� �̵��� �����ʹ� �����̵���ۿ��� �����˴ϴ�.
	/// </summary>
	/// <param name="out_">��¹����� ������</param>
	/// <param name="maxSize_">��¹����� ũ��</param>
	/// <returns>����� �������� ũ��</returns>
	uint32_t Dequeue(char* out_, const uint32_t maxSize_);

	/// <summary>
	/// �����̵���� ���� �����͸� Ư�� ũ�⸸ŭ �����մϴ�.
	/// ���� �������� ũ�⺸�� ū �����͸� �����Ϸ��� �ϸ� �����մϴ�.
	/// </summary>
	/// <param name="size_">������ �������� ũ��</param>
	/// <returns>���� ���� ����</returns>
	bool Pop(const uint32_t size_);

	/// <summary>
	/// �����̵������ ���� ���ο� �ִ� 4����Ʈ�� uint32_t�� �Ľ��Ͽ� ��ȯ�մϴ�.
	/// �����̵���ۿ� ���� �������� ����� �޽����� ũ�⸦ ������ ���̱� ������ �ۼ�.
	/// ���� �����Ͱ� 5����Ʈ �̸��� ��� 0�� ��ȯ�մϴ�. (������� ������ ���̱� ����)
	/// 
	/// ���Ź��ۿ��� ����� ��.
	/// �۽Ź��ۿ����� ����� ������� ���� �� �ִ� �ִ�ũ���� �����͸� �۽� �����Ѵ�.
	/// </summary>
	/// <returns>ù �޽����� ũ��</returns>
	uint32_t Peek();

	/// <summary>
	/// �����̵���ۿ� ��� ��� �������� ũ���� ���� ��ȯ�մϴ�.
	/// </summary>
	/// <returns>��� ��� �������� ũ�� ��</returns>
	uint32_t GetSize();

	bool IsEmpty();

	/// <summary>
	/// �����̵���۰� ���� �� �ִ� �Ѱ�뷮�� ��ȯ�մϴ�.
	/// Init(const uint64_t)�� ���� �Ѱ�뷮�� ������ �� �ֽ��ϴ�. 
	/// </summary>
	/// <returns>������ �Ѱ�뷮</returns>
	uint32_t GetCapacity();

	/// <summary>
	/// ���۸� ��ȯ�մϴ�.
	/// 
	/// WSABUF.buf�� �����ϱ����� char*Ÿ������ �����Ͽ�����
	/// �ܺο��� �����͸� ���� �������� �ʵ��� ����.
	/// </summary>
	/// <returns>���� ������</returns>
	char* GetBuf();

private:
	char* m_pData;
	uint32_t m_Size;
	uint32_t m_Capacity;
};