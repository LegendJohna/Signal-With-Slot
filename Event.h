#include <map>
#include "EventHandler.h"
using std::map;
using std::pair;
template<typename...Args>
class Event
{
	using Handler = EventHandlerInterface<Args...>*;
	using Address = pair<void*, void*>;
private:
	map<Address, Handler> HandlerList;
public:
	//���캯��ȫ����ʼ��Ϊ��
	~Event()
	{
		for (auto it = HandlerList.begin(); it != HandlerList.end(); it++)
		{
			delete (it->second);
		}
		HandlerList.clear();
	}
	//�º�������̬������lambda���ʽ
	template <typename T>
	void connect(T func)
	{
		auto address = Address(nullptr, &func);
		if (HandlerList.count(address) == 0)
		{
			auto handler = new CommonEventHandler<T, Args...>(func);
			HandlerList.insert(pair<Address, Handler>(address, handler));
		}

	}
	//ȫ�ֺ���
	template <typename T>
	void connect(T* func)
	{
		auto address = Address(nullptr, func);
		if(HandlerList.count(address) == 0)
		{
			auto handler = new GlobalEventHandler<T*, Args...>(func);
			HandlerList.insert(pair<Address, Handler>(address, handler));
		}
		
	}
	//������Ա����
	template <typename T>
	void connect(T* receiver, void(T::* func)(Args...))
	{
		auto address = Address(receiver, &func);
		if (HandlerList.count(address) == 0)
		{
			auto handler = new ClassEventHandler<T, Args...>(receiver, func);
			HandlerList.insert(pair<Address, Handler>(address, handler));
		}
		
	}
	//�Ͽ���ͨ����
	template <typename T>
	void disconnect(T func)
	{
		 auto address = Address(nullptr, &func);
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
		auto address = Address(nullptr, func);
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
		auto address = Address(receiver, &func);
		if (HandlerList.count(address) == 1)
		{
			delete HandlerList.at(address);
			HandlerList.erase(address);
		}

	}
	//�����ź�Ҳ���Ǵ���֮ǰ����º���
	template<typename ...Srgs>
	void emit(Srgs&&...srgs)
	{
		for (auto it = HandlerList.begin(); it != HandlerList.end(); it++)
		{
			(*it->second)(srgs...);
		}
	}
};