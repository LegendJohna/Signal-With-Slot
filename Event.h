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
	//������ͨ�ĺ���
	map<void*, m_func> HandlerList;
	//�������ڲ�����
	map<address, m_func> ClassHandlerList;
public:
	//���캯��ȫ����ʼ��Ϊ��
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
	//�º�������̬������lambda���ʽ
	template <typename T>
	void connect(T func)
	{
		auto handler = new CommonEventHandler<T, Args...>(func);
		HandlerList.insert(pair<void*, m_func>(&func, handler));
	}
	//ȫ�ֺ���
	template <typename T>
	void connect(T* func)
	{
		auto handler = new GlobalEventHandler<T*, Args...>(func);
		HandlerList.insert(pair<void*, m_func>(func,handler));
	}
	//������Ա����
	template <typename T>
	void connect(T* receiver, void(T::* func)(Args...))
	{
		auto handler = new ClassEventHandler<T, Args...>(receiver, func);
		ClassHandlerList.insert(pair<address, m_func>(address(receiver, &func), handler));
	}


	//�Ͽ���ͨ����
	template <typename T>
	void disconnect(T func)
	{
		 delete HandlerList.at((void*)&func);
		 HandlerList.erase((void*)&func);
	}
	//�Ͽ�ȫ�ֺ���
	template <typename T>
	void disconnect(T* func)
	{
		delete HandlerList.at((void*)func);
		HandlerList.erase((void*)func);
	}

	//�Ͽ����ں���
	template <typename T>
	void disconnect(T* receiver, void(T::* func)(Args...))
	{
		delete ClassHandlerList.at(address(receiver,&func));
		ClassHandlerList.erase(address(receiver, &func));
	}

	//�����ź�Ҳ���Ǵ���֮ǰ����º���
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