#include <iostream>
#include <map>
#include "EventHandler.h"
template<typename...Args>
class Event
{
	using m_func = BasicalEventHandler<Args...>*;
private:
	std::map<int,m_func> HandlerList; 
	m_func m_handler;
	int defultKey;
public:
	//构造函数全部初始化为空
	Event()
	{
		m_handler = nullptr;
		defultKey = 0;
	}
	~Event()
	{
		for (auto it = HandlerList.begin(); it != HandlerList.end(); it++)
		{
			delete (it->second);
		}
		HandlerList.clear();
	}
	//仿函数，静态函数，lambda表达式
	template <typename T>
	int connect(T func)
	{
		auto handler = new CommonEventHandler<T, Args...>(func);
		defultKey++;
		HandlerList.insert(std::pair<int, m_func>(defultKey, handler));
		return defultKey;
	}
	//全局函数
	template <typename T>
	int connect(T* func)
	{
		auto handler = new GlobalEventHandler<T*, Args...>(func);
		defultKey++;
		HandlerList.insert(std::pair<int, m_func>(defultKey,handler));
		return defultKey;
	}
	//添加类成员函数
	template <typename T>
	int connect(T* receiver, void(T::* func)(Args...))
	{
		auto handler = new ClassEventHandler<T, Args...>(receiver, func);
		defultKey++;
		HandlerList.insert(std::pair<int, m_func>(defultKey, handler));
		return defultKey;
		
	}
	bool disconncet(int funcNumber)
	{
		delete HandlerList.at(funcNumber);
		HandlerList.erase(funcNumber);
		return HandlerList.count(funcNumber) == 1;
	}
	bool disconncet(const char* order)
	{
		if (order == "all"||order == "All" ||order == "ALL")
		{
			for (auto it = HandlerList.begin(); it != HandlerList.end(); it++)
			{
				delete (it->second);
			}
			HandlerList.clear();
		}
		return HandlerList.empty();
	}
	//发送信号也就是触发之前定义仿函数
	template<typename ...Args>
	void emit(Args&&...args)
	{
		for (auto it = HandlerList.begin(); it != HandlerList.end(); it++)
		{
			(*it->second)(args...);
		}
	}
};