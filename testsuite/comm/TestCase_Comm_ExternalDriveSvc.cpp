// The MIT License (MIT)

// Copyright (c) 2013 lailongwei<lailongwei@126.com>
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of 
// this software and associated documentation files (the "Software"), to deal in 
// the Software without restriction, including without limitation the rights to 
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of 
// the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS 
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER 
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include "comm/TestCase_Comm_ExternalDriveSvc.h"

namespace
{

const int OPCODE = 0;

class TestFacade : public LLBC_IFacade
{
public:
    TestFacade()
    : _recvTimes(0)
    {
        LLBC_PrintLine(">>> Facade ctor called!");
    }

    virtual ~TestFacade()
    {
        LLBC_PrintLine(">>> Facade dtor called!");
    }

public:
    virtual void OnInitialize()
    {
        LLBC_PrintLine("Service initialize");
    }

    virtual void OnDestroy()
    {
        LLBC_PrintLine("Service destroy");
    }

    virtual void OnSessionCreate(const LLBC_SessionInfo &sessionInfo)
    {
        LLBC_PrintLine("Session create: %s", sessionInfo.ToString().c_str());
    }

    virtual void OnSessionDestroy(const LLBC_SessionDestroyInfo &destroyInfo)
    {
        LLBC_PrintLine("Session destroy, info: %s", destroyInfo.ToString().c_str());
    }

    virtual void OnAsyncConnResult(const LLBC_AsyncConnResult &result)
    {
        LLBC_PrintLine("Async-Conn result: %s", result.ToString().c_str());
    }

    virtual void OnUpdate()
    {
        LLBC_PrintLine("Service update");
    }

    virtual void OnIdle(int idleTime)
    {
        LLBC_PrintLine("Service idle, idleTime: %d", idleTime);
    }

public:
    virtual void OnDataArrival(LLBC_Packet &packet)
    {
        LLBC_PrintLine("Session[%d] receive data, len: %d\n", 
            packet.GetSessionId(), packet.GetLength());

        if (++_recvTimes == 100)
        {
            LLBC_PrintLine("Facade received %d times, stop this service", _recvTimes);

            LLBC_PrintLine("Print any key to stop...");
            getchar();

            GetService()->Stop();

            return;
        }

        LLBC_Packet *resPacket = LLBC_New(LLBC_Packet);
        resPacket->SetHeader(packet, OPCODE, 0);
        resPacket->Write(packet.GetPayload(), packet.GetPayloadLength());

        GetService()->Send(resPacket);
    }

private:
    int _recvTimes;
};

}

TestCase_Comm_ExternalDriveSvc::TestCase_Comm_ExternalDriveSvc()
: _svc(LLBC_IService::Create(LLBC_IService::Normal, "ExternalDriveSvc"))
{
}

TestCase_Comm_ExternalDriveSvc::~TestCase_Comm_ExternalDriveSvc()
{
    LLBC_Delete(_svc);
}

int TestCase_Comm_ExternalDriveSvc::Run(int argc, char *argv[])
{
    LLBC_PrintLine("ExternalDrive type service test:");

    // Parse arguments.
    if (argc < 3)
    {
        LLBC_PrintLine("Startup argument error, eg: ./a ip port");
        return -1;
    }

    const char *ip = argv[1];
    const uint16 port = LLBC_Str2Int32(argv[2]);

    // Create facade and register it.
    TestFacade *facade = LLBC_New(TestFacade);
    _svc->RegisterFacade(facade);
    _svc->Subscribe(OPCODE, facade, &TestFacade::OnDataArrival);

    // Set drive mode and start it.
    _svc->SetDriveMode(LLBC_IService::ExternalDrive);
    if (_svc->Start() != LLBC_OK)
    {
        LLBC_PrintLine("Service startup failed, reason: %s", LLBC_FormatLastError());
        return -1;
    }

    // Create listen port.
    int listenSid;
    if ((listenSid = _svc->Listen(ip, port)) == 0)
    {
        LLBC_PrintLine("Listen in %s:%d failed", ip, port);
        return -1;
    }

    // Connect it.
    int connSid;
    if ((connSid = _svc->Connect(ip, port)) == 0)
    {
        LLBC_PrintLine("Connect to %s:%d failed", ip, port);
        return -1;
    }

    // Send data(Send(LLBC_Packet *) method).
    LLBC_Packet *packet = LLBC_New(LLBC_Packet);
    packet->SetHeader(connSid, OPCODE, 0);
    *packet <<"Hello World!";
    _svc->Send(packet);

    // Send data(high level Send() methods).
    _svc->Send(connSid, OPCODE, 3, 0);
    _svc->Send(connSid, OPCODE, 3.0, 0);
    _svc->Send(connSid, OPCODE, reinterpret_cast<LLBC_ICoder *>(NULL), 0);

    while (_svc->IsStarted())
        _svc->OnSvc(true);

    return 0;
}
