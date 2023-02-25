#pragma once
#include <map>
#include <unordered_set>
#include <mutex>
using std::unordered_set;
using std::map;
using std::pair;
using std::mutex;
/*
	��������ڶ���������ӿ�
	Ϊ��ʵ������function�����Ĺ���
	���õķ����Ƕ�̬+ģ��
*/
//�ɱ�ģ�����
template <typename ...Args>
class EventHandlerInterface
{
private:
	EventHandlerInterface(const EventHandlerInterface&) = delete;
	EventHandlerInterface& operator=(const EventHandlerInterface& event) = delete;
public:
	EventHandlerInterface() {};
	virtual ~EventHandlerInterface() {};
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
	OrdinaryEventHandler(Function f)
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
	GlobalEventHandler(FunctionPointer handler)
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
static unordered_set<void*> ObjectList;
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
//Event
template<typename...Args>
class Event
{

	using Handler = EventHandlerInterface<Args...>*;
	using Address = pair<void*, void*>;
private:
	mutex MapMutex;
	map<Address, Handler> HandlerList;
public:
	~Event()
	{
		disconnectAllConnection();
	}
	//�º�������̬������lambda���ʽ
	template <typename T>
	void connect(T func)
	{
		MapMutex.lock();
		auto address = Address(nullptr, &func);
		if (HandlerList.count(address) == 0)
		{
			auto handler = new OrdinaryEventHandler<T, Args...>(func);
			HandlerList.insert(pair<Address, Handler>(address, handler));
		}
		MapMutex.unlock();
	}
	//ȫ�ֺ���
	template <typename T>
	void connect(T* func)
	{
		MapMutex.lock();
		auto address = Address(nullptr, func);
		if (HandlerList.count(address) == 0)
		{
			auto handler = new GlobalEventHandler<T*, Args...>(func);
			HandlerList.insert(pair<Address, Handler>(address, handler));
		}
		MapMutex.unlock();
	}
	//������Ա����
	template <typename T>
	void connect(T* receiver, void(T::* func)(Args...))
	{
		MapMutex.lock();
		//�Ѻ���ָ������ĵ�ַȡ������Ϊ��ʶ
		//��Ϊ������Ա����ָ��Ƚ����⣬����func������ʵ��Ĳ��ǵ�ַ
		//Ҳ����ֱ��ת��Ϊvoid*ֻ��ͨ�������ֶ�ȡ������
		void* buffer = nullptr;
		memcpy(&buffer, &func, sizeof(func));
		auto address = Address(receiver, buffer);
		if (HandlerList.count(address) == 0)
		{
			auto handler = new ClassEventHandler<T, Args...>(receiver, func);
			HandlerList.insert(pair<Address, Handler>(address, handler));
		}
		MapMutex.unlock();
	}
	//�Ͽ���ͨ����
	template <typename T>
	void disconnect(T func)
	{
		MapMutex.lock();
		auto address = Address(nullptr, &func);
		if (HandlerList.count(address) == 1)
		{
			delete HandlerList.at(address);
			HandlerList.erase(address);
		}
		MapMutex.unlock();
	}
	//�Ͽ�ȫ�ֺ���
	template <typename T>
	void disconnect(T* func)
	{
		MapMutex.lock();
		auto address = Address(nullptr, func);
		if (HandlerList.count(address) == 1)
		{
			delete HandlerList.at(address);
			HandlerList.erase(address);
		}
		MapMutex.unlock();
	}
	//�Ͽ����ں���
	template <typename T>
	void disconnect(T* receiver, void(T::* func)(Args...))
	{
		MapMutex.lock();
		void* buffer = nullptr;
		memcpy(&buffer, &func, sizeof(func));   //ǿ��ȡ������ָ���ڲ��ĵ�ַ
		auto address = Address(receiver, buffer);
		if (HandlerList.count(address) == 1)
		{
			delete HandlerList.at(address);
			HandlerList.erase(address);
		}
		MapMutex.unlock();
	}
	void disconnectAllConnection()
	{
		MapMutex.lock();
		for (auto it = HandlerList.begin(); it != HandlerList.end(); it++)
		{
			delete (it->second);
		}
		HandlerList.clear();
		MapMutex.unlock();
	}
	//�����ź�Ҳ���Ǵ���֮ǰ����º���
	template<typename ...Srgs>
	void emit(Srgs&&...srgs)
	{
		MapMutex.lock();
		for (auto it = HandlerList.begin(); it != HandlerList.end();)
		{
			if (it->first.first == nullptr)    //��һ������Ϊnullptr˵���ǲ������ں���,����ִ��
			{
				(*it->second)(srgs...);
				it++;
			}
			else if (ObjectList.count(it->first.first) == 0) //��object�����Ҳ���������,˵���Ǳ�delete��
			{                                                //�Զ�У����ɾ����Ԫ��
				delete (it->second);
				HandlerList.erase(it++);
			}
			else    //�������˵�������ҵ���������
			{
				(*it->second)(srgs...);
				it++;
			}
		}
		MapMutex.unlock();
	}
};



