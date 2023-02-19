#include <iostream>
#include <map>
#include <string>
#include "EventHandler.h"
using std::map;
using std::pair;
template<typename...Args>
class Event
{
	using m_func = BasicalEventHandler<Args...>*;
	using address = pair<void*, void*>;
private:
	//储存普通的函数
	map<void*, m_func> HandlerList;
	//储存类内部函数
	map<address, m_func> ClassHandlerList;
public:
	//构造函数全部初始化为空
	~Event()
	{
		for (auto it = HandlerList.begin(); it != HandlerList.end(); it++)
		{
			delete (it->second);
		}
		for (auto it = ClassHandlerList.begin(); it != ClassHandlerList.end(); it++)
		{
			delete (it->second);
		}
		HandlerList.clear();
		ClassHandlerList.clear();
	}
	//仿函数，静态函数，lambda表达式
	template <typename T>
	void connect(T func)
	{
		auto handler = new CommonEventHandler<T, Args...>(func);
		HandlerList.insert(pair<void*, m_func>(&func, handler));
	}
	//全局函数
	template <typename T>
	void connect(T* func)
	{
		auto handler = new GlobalEventHandler<T*, Args...>(func);
		HandlerList.insert(pair<void*, m_func>(func,handler));
	}
	//添加类成员函数
	template <typename T>
	void connect(T* receiver, void(T::* func)(Args...))
	{
		auto handler = new ClassEventHandler<T, Args...>(receiver, func);
		ClassHandlerList.insert(pair<address, m_func>(address(receiver, &func), handler));
	}


	//断开普通函数
	template <typename T>
	void disconnect(T func)
	{
		 delete HandlerList.at((void*)&func);
		 HandlerList.erase((void*)&func);
	}
	//断开全局函数
	template <typename T>
	void disconnect(T* func)
	{
		delete HandlerList.at((void*)func);
		HandlerList.erase((void*)func);
	}

	//断开类内函数
	template <typename T>
	void disconnect(T* receiver, void(T::* func)(Args...))
	{
		delete ClassHandlerList.at(address(receiver,&func));
		ClassHandlerList.erase(address(receiver, &func));
	}

	//发送信号也就是触发之前定义仿函数
	template<typename ...Args>
	void emit(Args&&...args)
	{
		for (auto it = HandlerList.begin(); it != HandlerList.end(); it++)
		{
			(*it->second)(args...);
		}
		for (auto it = ClassHandlerList.begin(); it != ClassHandlerList.end(); it++)
		{
			(*it->second)(args...);
		}
	}
};