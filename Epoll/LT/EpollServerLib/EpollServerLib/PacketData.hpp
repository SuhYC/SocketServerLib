#pragma once

#include "Define.hpp"

#include <stdint.h>
#include <string>
#include <new>
#include <iostream>
#include <cstring>


class PacketData
{
public:
	PacketData();
	~PacketData();

	/// <summary>
	/// ���ۿ� ���� �����͸� �ʱ�ȭ�մϴ�.
	/// (���۸� ���������� ����)
	/// </summary>
	void Clear();

	/// <summary>
	/// �Ҵ�� ���� �޸𸮸� �����մϴ�.
	/// </summary>
	void Free();

	/// <summary>
	/// ���۸� ���Ҵ��մϴ�.
	/// ���� ���۰� �־��ٸ� �����ϸ� �����ʹ� ���ŵ˴ϴ�.
	/// </summary>
	/// <param name="size_"></param>
	/// <returns></returns>
	bool Allocate(uint32_t size_ = PACKET_SIZE);

	/// <summary>
	/// ��ü ���� ����ȭ�� ������ ���(�����޸�x) �����ϵǸ�,
	/// ���ۿ� �����͸� �����մϴ�.
	/// ��Ʈ��ũIO�� ����� �� �ֵ��� ��������� �����˴ϴ�.
	/// 
	/// POD��ü�� ������ּ���.
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <param name="objData_"></param>
	/// <returns></returns>
	template <typename T>
	typename std::enable_if<std::is_trivially_copyable<T>::value&& std::is_standard_layout<T>::value, bool>::type
		Init(InfoCode infoCode_, uint32_t reqNo_, T& objData_)
	{
		uint32_t size = sizeof(T);
		uint32_t totalSize = size + sizeof(InfoHeader);
		if (totalSize > PACKET_SIZE)
		{
			return false;
		}

		int32_t nInfoCode = static_cast<int32_t>(infoCode_);
		m_Size = 0;

		// totalSize
		memcpy(m_pData + m_Size, &totalSize, sizeof(uint32_t));
		m_Size += sizeof(uint32_t);

		// infoCode
		memcpy(m_pData + m_Size, &nInfoCode, sizeof(int32_t));
		m_Size += sizeof(int32_t);

		// reqNo
		memcpy(m_pData + m_Size, &reqNo_, sizeof(uint32_t));
		m_Size += sizeof(uint32_t);

		// payload
		if (size > 0)
		{
			memcpy(m_pData + m_Size, &objData_, sizeof(T));
			m_Size += sizeof(T);
		}

		return true;
	}

	/// <summary>
	/// ���ۿ� �����͸� �����մϴ�
	/// ��Ʈ��ũIO�� ����� �� �ֵ���
	/// ��������� �����˴ϴ�.
	/// </summary>
	/// <param name="strData_"></param>
	/// <returns></returns>
	bool Init(InfoCode infoCode_, uint32_t reqNo_, std::string& strData_);

	/// <summary>
	/// ���ۿ� �����͸� �����մϴ�
	/// ��Ʈ��ũIO�� ����� �� �ֵ���
	/// ��������� �����˴ϴ�.
	/// 
	/// dataSize_�� pData_�� ũ�⸦ ���� �ʵ��� ����.
	/// </summary>
	/// <param name="infoCode_"></param>
	/// <param name="reqNo_"></param>
	/// <param name="pData_"></param>
	/// <param name="dataSize_"></param>
	/// <returns></returns>
	bool Init(InfoCode infoCode_, uint32_t reqNo_, const char* pData_, uint32_t dataSize_);

	/// <summary>
	/// ���̷ε� ���� �����ڵ�� ��û��ȣ�� �����մϴ�.
	/// </summary>
	/// <param name="infoCode_"></param>
	/// <param name="reqNo_"></param>
	/// <returns></returns>
	bool Init(InfoCode infoCode_, uint32_t reqNo_);

	/// <summary>
	/// ���� ��ȯ
	/// </summary>
	/// <returns>������ ������</returns>
	char* GetData();

	/// <summary>
	/// ���ۿ� ����� �������� ���� ��ȯ
	/// </summary>
	/// <returns>����� �������� ����</returns>
	uint32_t GetSize() const;

private:
	char* m_pData;
	uint32_t m_Size;
	uint32_t m_Capacity;
};