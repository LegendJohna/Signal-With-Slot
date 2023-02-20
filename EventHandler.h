#pragma once
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
class CommonEventHandler :public EventHandlerInterface<Args...>
{
private:
	Function* m_Handler;
public:
	CommonEventHandler(Function f)
	{
		m_Handler = new Function(f);
	};
	void operator()(Args&...args) 
	{
		(*m_Handler)(args...);
	} 
	~CommonEventHandler()
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
