#include <stdafx.h>
#include "host_timer_dispatcher.h"

#include <js_objects/global_object.h>
#include <js_utils/js_error_helper.h>

#include <user_message.h>
#include <message_manager.h>

HostTimerDispatcher::HostTimerDispatcher()
{
    m_curTimerId = 1;
    m_hTimerQueue = CreateTimerQueue();
    createThread();
}

HostTimerDispatcher::~HostTimerDispatcher()
{
    stopThread();
}

HostTimerDispatcher& HostTimerDispatcher::Get()
{
    static HostTimerDispatcher timerDispatcher;
    return timerDispatcher;
}

uint32_t HostTimerDispatcher::setInterval( HWND hWnd, uint32_t delay, JSContext* cx, JS::HandleFunction jsFunction )
{
    return createTimer( hWnd, delay, true, cx, jsFunction );
}

uint32_t HostTimerDispatcher::setTimeout( HWND hWnd, uint32_t delay, JSContext* cx, JS::HandleFunction jsFunction )
{
    return createTimer( hWnd, delay, false, cx, jsFunction );
}

void HostTimerDispatcher::killTimer( uint32_t timerId )
{
    std::lock_guard<std::mutex> lock( m_timerMutex );

    auto it = m_timerMap.find( timerId );
    if ( m_timerMap.end() != it )
    {
        it->second->stop();
    }
}

void HostTimerDispatcher::onPanelUnload( HWND hWnd )
{
    std::vector<uint32_t> timersToDelete;

    {
        std::lock_guard<std::mutex> lock( m_timerMutex );

        for ( const auto& [timerId, timer] : m_timerMap )
        {
            if ( timer->GetHwnd() == hWnd )
            {
                timersToDelete.push_back( timerId );
            }
        }
    }

    for ( auto timerId : timersToDelete )
    {
        killTimer( timerId );
    }
}

void HostTimerDispatcher::onTimerExpire( uint32_t timerId )
{
    std::lock_guard<std::mutex> lock( m_timerMutex );

    m_timerMap.erase( timerId );
}

void HostTimerDispatcher::onTimerStopRequest( HWND hWnd, HANDLE hTimer, uint32_t timerId )
{
    std::unique_lock<std::mutex> lock( m_threadTaskMutex );

    ThreadTask threadTask = {};
    threadTask.taskId = ThreadTaskId::killTimerTask;
    threadTask.hWnd = hWnd;
    threadTask.hTimer = hTimer;
    threadTask.timerId = timerId;

    m_threadTaskList.push_front( threadTask );
    m_cv.notify_one();
}

uint32_t HostTimerDispatcher::createTimer( HWND hWnd, uint32_t delay, bool isRepeated, JSContext* cx, JS::HandleFunction jsFunction )
{
    if ( !jsFunction )
    {
        return 0;
    }

    std::lock_guard<std::mutex> lock( m_timerMutex );

    uint32_t id = m_curTimerId++;
    while ( m_timerMap.count( id ) )
    {
        id = m_curTimerId++;
    }

    JS::RootedValue jsFuncValue( cx, JS::ObjectValue( *JS_GetFunctionObject( jsFunction ) ) );
    m_timerMap.emplace( id, std::make_unique<HostTimer>( hWnd, id, delay, isRepeated, std::make_unique<HostTimerTask>( cx, jsFuncValue ) ) );

    if ( !m_timerMap[id]->start( m_hTimerQueue ) )
    {
        m_timerMap.erase( id );
        return 0;
    }

    return id;
}

void HostTimerDispatcher::createThread()
{
    m_thread = std::make_unique<std::thread>( &HostTimerDispatcher::threadMain, this );
}

void HostTimerDispatcher::stopThread()
{
    if ( !m_thread )
    {
        return;
    }

    {
        std::unique_lock<std::mutex> lock( m_threadTaskMutex );
        ThreadTask threadTask = {};
        threadTask.taskId = ThreadTaskId::shutdownTask;

        m_threadTaskList.push_front( threadTask );
        m_cv.notify_one();
    }

    if ( m_thread->joinable() )
    {
        m_thread->join();
    }

    m_thread.reset();
}

void HostTimerDispatcher::threadMain()
{
    while ( true )
    {
        ThreadTask threadTask;
        {
            std::unique_lock<std::mutex> lock( m_threadTaskMutex );
            
            m_cv.wait( lock, [&threadTaskList = m_threadTaskList] { return !threadTaskList.empty(); } );

            threadTask = m_threadTaskList.front();
            m_threadTaskList.pop_front();
        }

        switch ( threadTask.taskId )
        {
        case ThreadTaskId::killTimerTask:
        {
            (void)DeleteTimerQueueTimer( m_hTimerQueue, threadTask.hTimer, INVALID_HANDLE_VALUE );
            onTimerExpire( threadTask.timerId );
            break;
        }
        case ThreadTaskId::shutdownTask:
        {
            (void)DeleteTimerQueueEx( m_hTimerQueue, INVALID_HANDLE_VALUE );
            m_hTimerQueue = nullptr;
            return;
        }
        default:
        {
            assert( 0 );
            break;
        }
        }
    }
}

HostTimerTask::HostTimerTask( JSContext* cx, JS::HandleValue funcValue )
    : JsAsyncTaskImpl( cx, funcValue )
{
}

bool HostTimerTask::InvokeJsImpl( JSContext* cx, JS::HandleObject jsGlobal, JS::HandleValue funcValue )
{
    JS::RootedFunction rFunc( cx, JS_ValueToFunction( cx, funcValue ) );

    JS::RootedValue dummyRetVal( cx );
    return JS::Call( cx, jsGlobal, rFunc, JS::HandleValueArray::empty(), &dummyRetVal );
}

HostTimer::HostTimer( HWND hWnd, uint32_t id, uint32_t delay, bool isRepeated, std::shared_ptr<HostTimerTask> task )
    : hWnd_( hWnd )
    , delay_( delay )
    , isRepeated_( isRepeated )
    , id_( id )
    , task_( task )
{
    assert( task );
}

bool HostTimer::start( HANDLE hTimerQueue )
{
    return !!CreateTimerQueueTimer(
        &hTimer_,
        hTimerQueue,
        HostTimer::timerProc,
        this,
        delay_,
        isRepeated_ ? delay_ : 0,
        WT_EXECUTEINTIMERTHREAD | ( isRepeated_ ? 0 : WT_EXECUTEONLYONCE ) );
}

void HostTimer::stop()
{
    isStopRequested_ = true;
}

VOID CALLBACK HostTimer::timerProc( PVOID lpParameter, BOOLEAN /*TimerOrWaitFired*/ )
{
    HostTimer* timer = reinterpret_cast<HostTimer*>( lpParameter );

    if ( timer->isStopped_ )
    {
        return;
    }

    if ( timer->isStopRequested_ )
    {
        timer->isStopped_ = true;
        HostTimerDispatcher::Get().onTimerStopRequest( timer->hWnd_, timer->hTimer_, timer->id_ );

        return;
    }

    auto postTimerTask = [&timer] {
        smp::panel::message_manager::instance().post_callback_msg( timer->hWnd_,
                                                                   smp::CallbackMessage::internal_timer_proc,
                                                                   std::make_unique<smp::panel::CallbackDataImpl<std::shared_ptr<HostTimerTask>>>( timer->task_ ) );
    };

    if ( timer->isRepeated_ )
    {
        postTimerTask();
    }
    else
    {
        timer->isStopped_ = true;
        postTimerTask();
        HostTimerDispatcher::Get().onTimerStopRequest( timer->hWnd_, timer->hTimer_, timer->id_ );
    }
}

HWND HostTimer::GetHwnd() const
{
    return hWnd_;
}

HostTimerTask& HostTimer::GetTask()
{
    return *task_;
}
