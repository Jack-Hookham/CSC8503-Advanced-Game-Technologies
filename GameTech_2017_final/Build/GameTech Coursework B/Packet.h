#pragma once

#include <ncltech\NetworkBase.h>

//Start at 1
//0 used for bad packet
enum PacketType
{
	PACKET_BAD,
	PACKET_MESSAGE,
	PACKET_MAZE_PARAMS,
	PACKET_MAZE_DATA
};

//A packet contains a type so that it can be identified and processed correctly
//as well as a char* of data to be processed
class Packet
{
public:
	Packet(int type)
	{
		m_packetType = type;
		m_data = "";
	}
	~Packet() {};

	//Add data onto the end of the current data
	template<typename T>
	void AddData(T data)
	{
		std::ostringstream oss;
		oss << m_data << data << " ";
		m_data = _strdup(oss.str().c_str());
	}

	inline const int GetPacketType() const { return m_packetType; }
	inline const char* GetData() const { return m_data; }

private:
	int m_packetType;
	char* m_data;
};