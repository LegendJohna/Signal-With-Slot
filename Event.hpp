#pragma once
#include <map>
#include <unordered_set>
#include <mutex>
#include <iostream>
/*
	这个类是在定义抽象函数接口
	为了实现类似function那样的功能
	采用的方法是多态+模板
*/
//可变模板参数
template <typename ...Args>
class EventHandlerInterface
{
private:
	EventHandlerInterface(const EventHandlerInterface&) = delete;
	EventHandlerInterface& operator=(const EventHandlerInterface& event) = delete;
public:
	EventHandlerInterface() {};
	virtual ~EventHandlerInterface() {};
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
//定义全局函数接口,事实上全局函数也可以使用第一个接口
//但是没必要new一个拷贝,我们为它专门定义一个接口
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
using std::map;
using std::pair;
using std::mutex;
using std::unordered_set;
static unordered_set<void*> ObjectList;

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
	//仿函数，静态函数，lambda表达式
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
	//全局函数
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
	//添加类成员函数
	template <typename T>
	void connect(T* receiver, void(T::* func)(Args...))
	{
		MapMutex.lock();
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
	//断开普通函数
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
	//断开全局函数
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
	//断开类内函数
	template <typename T>
	void disconnect(T* receiver, void(T::* func)(Args...))
	{
		MapMutex.lock();
		void* buffer = nullptr;
		memcpy(&buffer, &func, sizeof(func));
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
	void disconnectAllReceiver(void* receiver)
	{
		MapMutex.lock();
		for (auto it = HandlerList.begin(); it != HandlerList.end();)
		{
			if (receiver == it->first.first)
			{
				delete (it->second);
				HandlerList.erase(it++);
			}
			else
			{
				it++;
			}
		}
		MapMutex.unlock();
	}
	//发送信号也就是触发之前定义仿函数
	template<typename ...Srgs>
	void emit(Srgs&&...srgs)
	{
		MapMutex.lock();
		for (auto it = HandlerList.begin(); it != HandlerList.end();)
		{
			if (it->first.first == nullptr)
			{
				(*it->second)(srgs...);
				it++;
			}
			else
			{
				if (ObjectList.count(it->first.first) == 0)
				{
					delete (it->second);
					HandlerList.erase(it++);
				}
				else
				{
					(*it->second)(srgs...);
					it++;
				}
			}
			
		}
		MapMutex.unlock();
	}
};

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

