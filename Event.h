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
		if(HandlerList.count(address) == 0)
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
		//ͬ��ȡ�������Ա����ָ�����������
		//ʹ��mecmpy���޷�ȡ����
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
	//�����ź�Ҳ���Ǵ���֮ǰ����º���
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