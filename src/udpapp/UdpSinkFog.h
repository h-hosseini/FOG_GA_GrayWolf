//
// Copyright (C) 2004 OpenSim Ltd.
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef _FOGFN_SRC_UDPSINKFOG_H
#define _FOGFN_SRC_UDPSINKFOG_H

#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
//Extra Begin
#include "src/ga/GA.h"
#include "src/grayWolf/GrayWolf.h"
//Extra End

namespace fogfn {
using namespace inet;


/**
 * Consumes and prints packets received from the Udp module. See NED for more info.
 */
class INET_API UdpSinkFog : public ApplicationBase, public UdpSocket::ICallback
{
  protected:
    enum SelfMsgKinds { START = 1, STOP };

    UdpSocket socket;
    int localPort = -1;
    L3Address multicastGroup;
    simtime_t startTime;
    simtime_t stopTime;
    cMessage *selfMsg = nullptr;
    int numReceived = 0;

    //Extra Begin
    std::string optimizerType;
    GA *gaOptimizer;
    GrayWolf *grayWolfOptimizer;
    //Fog node info
    union ChunkDataUnion {
        double doubleValue;
        uint8_t rawBytes[sizeof(double)]; //unsigned char rawBytes[sizeof(double)]; //uint64_t iValue;
    };
    typedef union ChunkDataUnion ChunkData;

    ChunkData processingCapacity; //processing capability of a fog node j

    ChunkData linkCapacity; //bandwidth between nodes in Mbps

    ChunkData powerIdle; // power consumption of fog node in idle state in watts
    ChunkData powerTransmission; // maximum power consumption during transmission in watts
    ChunkData powerProcessing; // maximum power consumption during processing in watts

    //Service Info
    ChunkData dataSize; // service size to calculate the processing time
    ChunkData requestSize; // size of service request to calculate the communication time
    ChunkData responseSize; // size of service response to calculate the communication time

    ChunkData serviceDeadline;

    //Extra End

  public:
    UdpSinkFog() {}
    virtual ~UdpSinkFog();

  protected:
    virtual void processPacket(Packet *msg);
    virtual void setSocketOptions();

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;
    virtual void refreshDisplay() const override;

    virtual void socketDataArrived(UdpSocket *socket, Packet *packet) override;
    virtual void socketErrorArrived(UdpSocket *socket, Indication *indication) override;
    virtual void socketClosed(UdpSocket *socket) override;

    virtual void processStart();
    virtual void processStop();

    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;
};

} // namespace fogfn

#endif

