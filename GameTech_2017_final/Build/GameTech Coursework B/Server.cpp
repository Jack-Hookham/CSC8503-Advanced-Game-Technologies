#include "Server.h"

Server::Server()
{

}

Server::~Server()
{
	system("pause");
	networkBase.Release();
}

void Server::Update(float dt)
{
		accum_time += dt;
		rotation += 0.5f * PI * dt;

		//Handle All Incoming Packets and Send any enqued packets
		networkBase.ServiceNetwork(dt, [&](const ENetEvent& evnt)
		{
			switch (evnt.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
				printf("- New Client Connected\n");
				break;

			case ENET_EVENT_TYPE_RECEIVE:
				printf("\t Client %d says: %s\n", evnt.peer->incomingPeerID, evnt.packet->data);
				enet_packet_destroy(evnt.packet);
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				printf("- Client %d has disconnected.\n", evnt.peer->incomingPeerID);
				break;
			}
		});

		//Broadcast update packet to all connected clients at a rate of UPDATE_TIMESTEP updates per second
		if (accum_time >= UPDATE_TIMESTEP)
		{
			//Packet data
			// - At the moment this is just a position update that rotates around the origin of the world
			//   though this can be any variable, structure or class you wish. Just remember that everything 
			//   you send takes up valuable network bandwidth so no sending every PhysicsObject struct each frame ;)
			accum_time = 0.0f;
			Vector3 pos = Vector3(
				cos(rotation) * 2.0f,
				1.5f,
				sin(rotation) * 2.0f);

			//Create the packet and broadcast it (unreliable transport) to all clients
			ENetPacket* position_update = enet_packet_create(&pos, sizeof(Vector3), 0);
			enet_host_broadcast(networkBase.m_pNetwork, 0, position_update);
		}

		Sleep(0);
}
