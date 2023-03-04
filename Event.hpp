#pragma once
#include <map>
#include <unordered_set>
#include <mutex>
#include <atomic>
/*
    这个类是在定义抽象函数接口
    为了实现类似function那样的功能
    采用的方法是多态+模板
*/
//可变模板参数
template <typename ...Args>
class EventHandlerInterface
{
public:
    EventHandlerInterface(const EventHandlerInterface&) = delete;
    EventHandlerInterface& operator=(const EventHandlerInterface& event) = delete;
    EventHandlerInterface() = default;
    virtual ~EventHandlerInterface() = default;
    //重载()是为了方便之后调用
    virtual void operator()(Args&...args) = 0;
};
//首先定义第一个函数接口，这个是为了给lambda,仿函数使用的接口，为了动态管理lambda内存
//通过new调用其拷贝构造函数，否则在lambda里面捕获的参数会销毁
template<typename Function, typename ...Args>
class OrdinaryEventHandler :public EventHandlerInterface<Args...>
{
private:
    Function* m_Handler;
public:
    explicit OrdinaryEventHandler(Function f)
    {
        m_Handler = new Function(f);
    };
    void operator()(Args&...args)
    {
        (*m_Handler)(args...);
    }
    ~OrdinaryEventHandler()
    {
        delete m_Handler;
    }
};
//定义全局函数接口,事实上全局函数也可以使用第一个接口
//但是没必要new一个拷贝,我们为它专门定义一个接口
template<typename FunctionPointer, typename ...Args>
class GlobalEventHandler :public EventHandlerInterface<Args...>
{
private:
    FunctionPointer m_Handler;
public:
    explicit GlobalEventHandler(FunctionPointer handler)
    {
        m_Handler = handler;
    };
    void operator()(Args&...args)
    {
        (*m_Handler)(args...);
    }
};
//接下来是类内部函数啦,这个需要多一个变量来存对象的指针
//类内部函数的接口
template<typename T, typename ...Args>
class ClassEventHandler :public EventHandlerInterface<Args...>
{
    using FunctionPointer = void(T::*)(Args...);
private:
    T* m_Receiver;
    FunctionPointer m_Handler;
public:
    ClassEventHandler(T* receiver, FunctionPointer handler)
    {
        m_Receiver = receiver;
        m_Handler = handler;
    }
    void operator()(Args&...args)
    {
        (m_Receiver->*m_Handler)(args...);
    }
};
//用来存object地址，用来进行校正
static std::unordered_set<void*> ObjectList;
class Object
{
public:
    Object()
    {
        ObjectList.insert((void*)this);
    }
    ~Object()
    {
        ObjectList.erase((void*)this);
    }
};
//多线程处理,采用双map来继续管理
//同时返回首位迭代器
template<typename T, typename U>
struct ConcurrentMapIterator
{
    typename std::map<T, U>::iterator begin;
    typename std::map<T, U>::iterator end;
};
template<typename T, typename U>
class ConcurrentMap
{
private:
    std::map<T, U> ReadMap;
    std::map<T, U> WriteMap;
    std::mutex MapMutex;
    bool writed = false;
    std::atomic<int> ReadingNum = 0;
    unsigned int m_Size = 0;
public:
    void insert(std::pair<T, U> pair)
    {
        std::lock_guard<std::mutex> lock(MapMutex);
        writed = true;
        WriteMap.insert(pair);
        m_Size++;
    }
    void erase(T key)
    {
        std::lock_guard<std::mutex> lock(MapMutex);
        writed = true;
        WriteMap.erase(key);
        m_Size--;
    }
    void clear()
    {
        std::lock_guard<std::mutex> lock(MapMutex);
        writed = true;
        WriteMap.clear();
        m_Size = 0;
    }
    U at(T key)
    {
        std::lock_guard<std::mutex> lock(MapMutex);
        return WriteMap.at(key);
    }
    int count(T key)
    {
        std::lock_guard<std::mutex> lock(MapMutex);
        return WriteMap.count(key);
    }
    int size()
    {
        return m_Size;
    }
    ConcurrentMapIterator<T, U> BeginAndEnd()
    {
        std::lock_guard<std::mutex> lock(MapMutex);
        if (writed)
        {
            ReadMap.clear();
            ReadMap.insert(WriteMap.begin(), WriteMap.end());
            writed = false;
        }
        return { ReadMap.begin(),ReadMap.end() };
    }
    bool ReadyRead()
    {
        while (true) 		//有两种情况可以同时读写,第一种情况没人读，那你随便读
        {				//第二种情况有人读，数据不会改，你也可以去读,如果都不满足就等待别人读完了再读
            if (ReadingNum == 0)
            {
                ReadingNum++;
                return true;
            }
            else if (ReadingNum > 0 && !writed)
            {
                ReadingNum++;
                return true;
            }
        }
    }
    void ReadEnd()
    {
        ReadingNum--;
    }
};
//事件处理
template<typename...Args>
class Event
{
    using Handler = EventHandlerInterface<Args...>*;
    using Address = std::pair<void*, void*>;//第一个是对象地址，第二个是函数地址
private:
    ConcurrentMap<Address, Handler> HandlerList;
    int LambdaID = 0;
public:
    ~Event()
    {
        disconnectAllConnection();
    }

    inline int ConnectionCount() { return HandlerList.size(); }
    //仿函数，静态函数，lambda表达式
    template <typename T>
    int connect(T func)
    {
        //分配一个id来断开连接,毕竟lambda式是没法寻找的
        LambdaID++;
        auto address = Address(nullptr, reinterpret_cast<void*>(LambdaID));
        if (HandlerList.count(address) == 0)
        {
            auto handler = new OrdinaryEventHandler<T, Args...>(func);
            HandlerList.insert(std::pair<Address, Handler>(address, handler));
            return LambdaID;
        }
        LambdaID--;
        return -1;
    }
    //全局函数
    template <typename T>
    void connect(T* func)
    {
        auto address = Address(nullptr, reinterpret_cast<void*>(func));
        if (HandlerList.count(address) == 0)
        {
            auto handler = new GlobalEventHandler<T*, Args...>(func);
            HandlerList.insert(std::pair<Address, Handler>(address, handler));
        }
    }
    //添加类成员函数
    template <typename T>
    void connect(T* receiver, void(T::* func)(Args...))
    {
        //把函数指针里面的地址取出来作为标识
        //因为这个类成员函数指针比较特殊，所以func里面其实存的不是地址
        //也不能直接转换为void*只能通过特殊手段取出来了
        void* buffer = nullptr;
        memcpy(&buffer, &func, sizeof(void*));
        auto address = Address(receiver, buffer);
        if (HandlerList.count(address) == 0)
        {
            auto handler = new ClassEventHandler<T, Args...>(receiver, func);
            HandlerList.insert(std::pair<Address, Handler>(address, handler));
        }
    }
    //断开普通函数
    void disconnect(int connectionID)
    {
        auto address = Address(nullptr, reinterpret_cast<void*>(connectionID));
        if (HandlerList.count(address) == 1)
        {
            delete HandlerList.at(address);
            HandlerList.erase(address);
        }
    }
    //断开全局函数
    template <typename T>
    void disconnect(T* func)
    {
        auto address = Address(nullptr, reinterpret_cast<void*>(func));
        if (HandlerList.count(address) == 1)
        {
            delete HandlerList.at(address);
            HandlerList.erase(address);
        }
    }
    //断开类内函数
    template <typename T>
    void disconnect(T* receiver, void(T::* func)(Args...))
    {
        void* buffer = nullptr;
        memcpy(&buffer, &func, sizeof(void*));   //强制取出函数指针内部的地址
        auto address = Address(receiver, buffer);
        if (HandlerList.count(address) == 1)
        {
            delete HandlerList.at(address);
            HandlerList.erase(address);
        }
    }
    void disconnectAllConnection()
    {
        auto result = HandlerList.BeginAndEnd();
        for (auto it = result.begin; it != result.end; it++)
        {
            delete (it->second);
        }
        HandlerList.clear();
    }
    //发送信号也就是触发之前定义仿函数
    template<typename ...Srgs>
    void emit(Srgs&&...srgs)
    {
        if (HandlerList.ReadyRead())
        {
            auto result = HandlerList.BeginAndEnd();
            for (auto it = result.begin; it != result.end;)
            {
                void* receiver = it->first.first;
                if (receiver == nullptr)    //第一个参数为nullptr说明是不是类内函数,正常执行
                {
                    (*it->second)(srgs...);
                    it++;
                }
                else if (ObjectList.count(receiver) == 0) //再object里面找不到发送者,说明是被delete了
                {                                                //自动校正，删除该元素
                    delete (it->second);
                    HandlerList.erase(it->first);
                    it++;
                }
                else    //这种情况说明，能找到正常触发
                {
                    (*it->second)(srgs...);
                    it++;
                }
            }
            HandlerList.ReadEnd();
        }
    }
};
