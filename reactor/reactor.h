#ifndef REACTOR_REACTOR_H_
#define REACTOR_REACTOR_H_

#include <stdint.h>
#include <unistd.h>
#include <sys/epoll.h>

namespace reactor
{
typedef unsigned int event_t;
enum
{
    kReadEvent    = 0x01,
    kWriteEvent   = 0x02, 
    kErrorEvent   = 0x04, 
    kEventMask    = 0xff  
};

typedef int handle_t;

class EventHandler
{
public:
    virtual handle_t GetHandle() const = 0;
    virtual void HandleRead() {}
    virtual void HandleWrite() {}
    virtual void HandleError() {}

protected:
    EventHandler() {}
    virtual ~EventHandler() {}
};

class ReactorImplementation;

class Reactor
{
public:
    Reactor();
    ~Reactor();

    int RegisterHandler(EventHandler * handler, event_t evt);
    int RemoveHandler(EventHandler * handler);
    void HandleEvents(int timeout = 0);

private:
    Reactor(const Reactor &);
    Reactor & operator=(const Reactor &);

private:
    ReactorImplementation * m_reactor_impl; 
};
}

#endif 
