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
	PACKET_PATH_REQUEST,
	PACKET_PATH,
	PACKET_UPDATE_AVATAR_IDX,
	PACKET_UPDATE_AVATAR_POS,
	PACKET_IS_MOVE
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
	Packet(int type) : m_packetType(type), m_data(NULL)
	{
	}

	~Packet()
	{
		delete[] m_data;
	};

	void SetData(std::string data)
	{
		//Delete old memory if it has been previously allocated
		if (m_data)
		{
			delete[] m_data;
		}
		//Allocate enough memory for the data plus a termination char
		m_data = new char[data.length() + 1];
		for (int i = 0; i < data.length(); ++i)
		{
			char from = data[i];
			m_data[i] = from;
		}
		//Add termination char
		m_data[data.length()] = '\0';
	}

	inline const int GetPacketType() const { return m_packetType; }
	inline const char* GetData() const { return m_data; }

	inline char* Data() { return m_data; }
	inline void InitData(char* data)
	{
		if (m_data) { delete[] m_data; } 
		m_data = data;
	}

private:
	int m_packetType;
	char* m_data;
};