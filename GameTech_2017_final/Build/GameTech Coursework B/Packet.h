#pragma once

#include <ncltech\NetworkBase.h>

//Start at 1
//0 used for bad packet checks
enum PacketType
{
	PACKET_BAD,
	PACKET_MESSAGE,
	PACKET_MAZE_PARAMS,
	PACKET_MAZE_DATA,
	PACKET_MOVE_START,
	PACKET_MOVE_END,
	PACKET_MOVE_CLIENT,
	PATH_REQUEST_PACKET
};

enum MoveDirection
{
	MOVEMENT_UP,
	MOVEMENT_LEFT,
	MOVENENT_DOWN,
	MOVEMENT_RIGHT
};

//A packet contains a type so that it can be identified and processed correctly
//as well as a char* of data to be processed
class Packet
{
public:
	Packet(int type) : m_packetType(type), m_data("")
	{
	}

	~Packet()
	{
		delete[] m_data;
	};

	//Add data onto the end of the current data
	template<typename T>
	void AddDataSpaced(T data)
	{
		std::ostringstream oss;
		oss << m_data << data << " ";
		m_data = _strdup(oss.str().c_str());
	}

	template<typename T>
	void AddData(T data)
	{
		std::ostringstream oss;
		oss << m_data << data;
		m_data = _strdup(oss.str().c_str());
	}

	inline const int GetPacketType() const { return m_packetType; }
	inline void SetData(char* data) { m_data = data; }
	inline const char* GetData() const { return m_data; }

	inline char* Data() { return m_data; }
	inline void InitData(char* data) { m_data = data; }

private:
	int m_packetType;
	char* m_data;
};