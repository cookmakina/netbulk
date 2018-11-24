#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <Poco/Net/TCPServer.h>
#include <Poco/Net/TCPServerConnection.h>
#include <Poco/Net/TCPServerConnectionFactory.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/SocketStream.h>
#include <Poco/Net/ServerSocket.h>
#include "async.h"
#include "arguments.h"


class BulkConnection: public Poco::Net::TCPServerConnection {
public:
    BulkConnection(const Poco::Net::StreamSocket& s, int _bulkSize): Poco::Net::TCPServerConnection(s), bulkSize(_bulkSize) { }

    virtual void run() override {
        Poco::Net::StreamSocket& ss = socket();
        context_id_t id = 0;

        try {
            id = connect((size_t)bulkSize);

            while (true) {
                char buffer[256];
                size_t numReceived = (size_t)ss.receiveBytes(buffer, sizeof(buffer));
                if (!numReceived)
                    break;

                receive(buffer, numReceived, id);
            }
        }
        catch (Poco::Exception& exc) {
            std::cerr << "BulkConnection: " << exc.displayText() << std::endl;
        }

        disconnect(id);
    }

private:
    int bulkSize;
};

class BulkServerConnectionFactory: public Poco::Net::TCPServerConnectionFactory {
public:
    explicit BulkServerConnectionFactory(int  _bulkSize): bulkSize(_bulkSize) {
    }

    virtual Poco::Net::TCPServerConnection* createConnection(const Poco::Net::StreamSocket& socket) override {
        return new BulkConnection(socket, bulkSize);
    }

private:
    int bulkSize;
};

int main(int argc, const char **argv) {
    ProgramArguments args(argc, argv);
    if (args.count() != 2)
        return 1;

    int port = args.getInt(0);
    int bulkSize = args.getInt(1);

    try {
        Poco::Net::ServerSocket ss((Poco::UInt16)port);
        ss.listen();

        Poco::Net::TCPServer srv(new BulkServerConnectionFactory(bulkSize), ss);
        srv.start();

        std::this_thread::sleep_for(std::chrono::seconds(6000));
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}