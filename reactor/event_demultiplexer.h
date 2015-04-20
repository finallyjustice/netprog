#ifndef REACTOR_EVENT_DEMULTIPLEXER_H_
#define REACTOR_EVENT_DEMULTIPLEXER_H_

#include <set>
#include <map>
#include "reactor.h"

namespace reactor
{
class EventDemultiplexer
{
public:
    virtual ~EventDemultiplexer() {}
    virtual int WaitEvents(std::map<handle_t, EventHandler *> * handlers,
                           int timeout = 0) = 0;

    virtual int RequestEvent(handle_t handle, event_t evt) = 0;
    virtual int UnrequestEvent(handle_t handle) = 0;
};

class EpollDemultiplexer : public EventDemultiplexer
{
public:
    EpollDemultiplexer();
    ~EpollDemultiplexer();

    virtual int WaitEvents(std::map<handle_t, EventHandler *> * handlers,
                           int timeout = 0);
    virtual int RequestEvent(handle_t handle, event_t evt);
    virtual int UnrequestEvent(handle_t handle);

private:
    int  m_epoll_fd; 
    int  m_fd_num;  
};
} 

#endif 
