#pragma once

#include <ncltech\NetworkBase.h>

enum PacketType
{
	PACKET_MESSAGE,
	PACKET_MAZE_PARAMS,
	PACKET_MAZE_DATA
};

struct PacketMazeParams
{
	int packetType = PACKET_MAZE_PARAMS;
	int mazeSize;
};

struct PacketMazeData
{
	int packetType = PACKET_MAZE_DATA;
	int mazeSize;
	bool* isWall;
};

struct PacketMessage
{
	int packetType = PACKET_MESSAGE;
	char* message;
};