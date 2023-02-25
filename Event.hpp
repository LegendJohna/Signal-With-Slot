#pragma once
#include <map>
#include <unordered_set>
#include <mutex>
using std::unordered_set;
using std::map;
using std::pair;
using std::mutex;
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
//用来存object地址，用来进行校正
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
		//把函数指针里面的地址取出来作为标识
		//因为这个类成员函数指针比较特殊，所以func里面其实存的不是地址
		//也不能直接转换为void*只能通过特殊手段取出来了
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
		memcpy(&buffer, &func, sizeof(func));   //强制取出函数指针内部的地址
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
	//发送信号也就是触发之前定义仿函数
	template<typename ...Srgs>
	void emit(Srgs&&...srgs)
	{
		MapMutex.lock();
		for (auto it = HandlerList.begin(); it != HandlerList.end();)
		{
			if (it->first.first == nullptr)    //第一个参数为nullptr说明是不是类内函数,正常执行
			{
				(*it->second)(srgs...);
				it++;
			}
			else if (ObjectList.count(it->first.first) == 0) //再object里面找不到发送者,说明是被delete了
			{                                                //自动校正，删除该元素
				delete (it->second);
				HandlerList.erase(it++);
			}
			else    //这种情况说明，能找到正常触发
			{
				(*it->second)(srgs...);
				it++;
			}
		}
		MapMutex.unlock();
	}
};



