#include <map>
#include <mutex>
#include "EventHandler.h"
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
			auto handler = new CommonEventHandler<T, Args...>(func);
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
		auto address = Address(receiver, &func);
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
		auto address = Address(receiver, &func);
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
		for (auto it = HandlerList.begin(); it != HandlerList.end(); it++)
		{
			if (receiver == it->first->first)
			{
				delete (it->second);
				HandlerList.erase(it);
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