#ifndef HV_UDP_SERVER_HPP_
#define HV_UDP_SERVER_HPP_

#include "hsocket.h"

#include "EventLoopThreadPool.h"
#include "Callback.h"
#include "Channel.h"

namespace hv {

class UdpServer {
public:
    UdpServer() {
    }

    virtual ~UdpServer() {
    }

    const EventLoopPtr& loop() {
        return loop_thread.loop();
    }

    //@retval >=0 bindfd, <0 error
    int createsocket(int port, const char* host = "0.0.0.0") {
        hio_t* io = hloop_create_udp_server(loop_thread.hloop(), host, port);
        if (io == NULL) return -1;
        channel.reset(new SocketChannel(io));
        return channel->fd();
    }
    void closesocket() {
        if (channel) {
            channel->close();
            channel = NULL;
        }
    }

    int startRecv() {
        assert(channel != NULL);
        channel->onread = [this](Buffer* buf) {
            if (onMessage) {
                onMessage(channel, buf);
            }
        };
        channel->onwrite = [this](Buffer* buf) {
            if (onWriteComplete) {
                onWriteComplete(channel, buf);
            }
        };
        return channel->startRead();
    }

    void start(bool wait_threads_started = true) {
        loop_thread.start(wait_threads_started, std::bind(&UdpServer::startRecv, this));
    }
    void stop(bool wait_threads_stopped = true) {
        loop_thread.stop(wait_threads_stopped);
    }

    int sendto(const void* data, int size, struct sockaddr* peeraddr = NULL) {
        if (channel == NULL) return -1;
        if (peeraddr) hio_set_peeraddr(channel->io(), peeraddr, SOCKADDR_LEN(peeraddr));
        return channel->write(data, size);
    }
    int sendto(Buffer* buf, struct sockaddr* peeraddr = NULL) {
        return sendto(buf->data(), buf->size(), peeraddr);
    }
    int sendto(const std::string& str, struct sockaddr* peeraddr = NULL) {
        return sendto(str.data(), str.size(), peeraddr);
    }

public:
    SocketChannelPtr        channel;
    // Callback
    MessageCallback         onMessage;
    WriteCompleteCallback   onWriteComplete;

private:
    EventLoopThread         loop_thread;
};

}

#endif // HV_UDP_SERVER_HPP_
