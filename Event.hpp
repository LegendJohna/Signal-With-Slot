#pragma once
#include <map>
#include <unordered_set>
#include <mutex>
#include <atomic>
/*
    ��������ڶ���������ӿ�
    Ϊ��ʵ������function�����Ĺ���
    ���õķ����Ƕ�̬+ģ��
*/
//�ɱ�ģ�����
template <typename ...Args>
class EventHandlerInterface
{
public:
    EventHandlerInterface(const EventHandlerInterface&) = delete;
    EventHandlerInterface& operator=(const EventHandlerInterface& event) = delete;
    EventHandlerInterface() = default;
    virtual ~EventHandlerInterface() = default;
    //����()��Ϊ�˷���֮�����
    virtual void operator()(Args&...args) = 0;
};
//���ȶ����һ�������ӿڣ������Ϊ�˸�lambda,�º���ʹ�õĽӿڣ�Ϊ�˶�̬����lambda�ڴ�
//ͨ��new�����俽�����캯����������lambda���沶��Ĳ���������
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
//����ȫ�ֺ����ӿ�,��ʵ��ȫ�ֺ���Ҳ����ʹ�õ�һ���ӿ�
//����û��Ҫnewһ������,����Ϊ��ר�Ŷ���һ���ӿ�
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
//�����������ڲ�������,�����Ҫ��һ��������������ָ��
//���ڲ������Ľӿ�
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
//������object��ַ����������У��
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
//���̴߳���,����˫map����������
//ͬʱ������λ������
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
        while (true) 		//�������������ͬʱ��д,��һ�����û�˶�����������
        {				//�ڶ���������˶������ݲ���ģ���Ҳ����ȥ��,�����������͵ȴ����˶������ٶ�
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
//�¼�����
template<typename...Args>
class Event
{
    using Handler = EventHandlerInterface<Args...>*;
    using Address = std::pair<void*, void*>;//��һ���Ƕ����ַ���ڶ����Ǻ�����ַ
private:
    ConcurrentMap<Address, Handler> HandlerList;
    int LambdaID = 0;
public:
    ~Event()
    {
        disconnectAllConnection();
    }

    inline int ConnectionCount() { return HandlerList.size(); }
    //�º�������̬������lambda���ʽ
    template <typename T>
    int connect(T func)
    {
        //����һ��id���Ͽ�����,�Ͼ�lambdaʽ��û��Ѱ�ҵ�
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
    //ȫ�ֺ���
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
    //������Ա����
    template <typename T>
    void connect(T* receiver, void(T::* func)(Args...))
    {
        //�Ѻ���ָ������ĵ�ַȡ������Ϊ��ʶ
        //��Ϊ������Ա����ָ��Ƚ����⣬����func������ʵ��Ĳ��ǵ�ַ
        //Ҳ����ֱ��ת��Ϊvoid*ֻ��ͨ�������ֶ�ȡ������
        void* buffer = nullptr;
        memcpy(&buffer, &func, sizeof(void*));
        auto address = Address(receiver, buffer);
        if (HandlerList.count(address) == 0)
        {
            auto handler = new ClassEventHandler<T, Args...>(receiver, func);
            HandlerList.insert(std::pair<Address, Handler>(address, handler));
        }
    }
    //�Ͽ���ͨ����
    void disconnect(int connectionID)
    {
        auto address = Address(nullptr, reinterpret_cast<void*>(connectionID));
        if (HandlerList.count(address) == 1)
        {
            delete HandlerList.at(address);
            HandlerList.erase(address);
        }
    }
    //�Ͽ�ȫ�ֺ���
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
    //�Ͽ����ں���
    template <typename T>
    void disconnect(T* receiver, void(T::* func)(Args...))
    {
        void* buffer = nullptr;
        memcpy(&buffer, &func, sizeof(void*));   //ǿ��ȡ������ָ���ڲ��ĵ�ַ
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
    //�����ź�Ҳ���Ǵ���֮ǰ����º���
    template<typename ...Srgs>
    void emit(Srgs&&...srgs)
    {
        if (HandlerList.ReadyRead())
        {
            auto result = HandlerList.BeginAndEnd();
            for (auto it = result.begin; it != result.end;)
            {
                void* receiver = it->first.first;
                if (receiver == nullptr)    //��һ������Ϊnullptr˵���ǲ������ں���,����ִ��
                {
                    (*it->second)(srgs...);
                    it++;
                }
                else if (ObjectList.count(receiver) == 0) //��object�����Ҳ���������,˵���Ǳ�delete��
                {                                                //�Զ�У����ɾ����Ԫ��
                    delete (it->second);
                    HandlerList.erase(it->first);
                    it++;
                }
                else    //�������˵�������ҵ���������
                {
                    (*it->second)(srgs...);
                    it++;
                }
            }
            HandlerList.ReadEnd();
        }
    }
};
