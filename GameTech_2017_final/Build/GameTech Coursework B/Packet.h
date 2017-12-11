#pragma once

#include <ncltech\NetworkBase.h>

enum PacketType
{
	PACKET_MESSAGE,
	PACKET_MAZE_PARAMS,
	PACKET_MAZE_DATA
};

//A packet contains a type so that it can be identified and processed correctly
//as well as a char* of data to be processed
struct Packet
{
	Packet(int type)
	{
		packetType = type;
	}

	int packetType;
	char* data;
};