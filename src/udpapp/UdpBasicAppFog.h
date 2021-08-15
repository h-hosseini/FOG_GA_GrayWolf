//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2004,2011 OpenSim Ltd.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef _FOGFN_SRC_UDPBASICAPPFOG_H
#define _FOGFN_SRC_UDPBASICAPPFOG_H

#include <vector>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/clock/ClockUserModuleMixin.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
//Extra Begin
#include "src/ga/GA.h"
//Extra End

namespace fogfn {
using namespace inet;


/**
 * UDP application. See NED for more info.
 */
class INET_API UdpBasicAppFog : public ClockUserModuleMixin<ApplicationBase>, public UdpSocket::ICallback
{
  protected:
    enum SelfMsgKinds { START = 1, SEND, STOP };

    // parameters
    std::vector<L3Address> destAddresses;
    std::vector<std::string> destAddressStr;
    int localPort = -1, destPort = -1;
    clocktime_t startTime;
    clocktime_t stopTime;
    bool dontFragment = false;
    const char *packetName = nullptr;

    // state
    UdpSocket socket;
    ClockEvent *selfMsg = nullptr;

    // statistics
    int numSent = 0;
    int numReceived = 0;

    //Extra Begin
    GA *gaOptimizer;
    //Fog node info
    union ChunkDataUnion {
        double doubleValue;
        uint8_t rawBytes[sizeof(double)];//unsigned char rawBytes[sizeof(double)]; //uint64_t iValue;
    };
    typedef union ChunkDataUnion ChunkData;

    ChunkData processingCapacity; //processing capability of a fog node j

    ChunkData linkCapacity; //bandwidth between nodes in Mbps

    ChunkData powerIdle; // power consumption of fog node in idle state in watts
    ChunkData powerTransmission; // maximum power consumption during transmission in watts
    ChunkData powerProcessing; // maximum power consumption during processing in watts

    //Service Info
    ChunkData cpuRequired;
    ChunkData ramRequired;
    ChunkData storageRequired;

    ChunkData dataSize; // service size to calculate the processing time
    ChunkData requestSize; // size of service request to calculate the communication time
    ChunkData responseSize; // size of service response to calculate the communication time

    ChunkData serviceDeadline;
    //Extra End

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;
    virtual void refreshDisplay() const override;

    // chooses random destination address
    virtual L3Address chooseDestAddr();
    virtual void sendPacket();
    virtual void processPacket(Packet *msg);
    virtual void setSocketOptions();

    virtual void processStart();
    virtual void processSend();
    virtual void processStop();

    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;

    virtual void socketDataArrived(UdpSocket *socket, Packet *packet) override;
    virtual void socketErrorArrived(UdpSocket *socket, Indication *indication) override;
    virtual void socketClosed(UdpSocket *socket) override;

  public:
    UdpBasicAppFog() {}
    ~UdpBasicAppFog();
};

} // namespace fogfn

#endif

