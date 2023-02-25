#include <map>
#include <mutex>
#include "EventHandler.h"
#include <iostream>
using std::map;
using std::pair;
using std::mutex;
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
		if(HandlerList.count(address) == 0)
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
		void* buffer;
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
		//同样取出来类成员函数指针里面的内容
		//使用mecmpy就无法取出来
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
		for (auto it = HandlerList.begin(); it != HandlerList.end(); it++)
		{
			(*it->second)(srgs...);
		}
		MapMutex.unlock();
	}
};
