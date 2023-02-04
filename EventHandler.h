#pragma once
/*
	��������ڶ���������ӿ�
	Ϊ��ʵ������function�����Ĺ���
	���õķ����Ƕ�̬+ģ��
*/
//�ɱ�ģ�����
template <typename ...Args>
class BasicalEventHandler
{
private:
	BasicalEventHandler(const BasicalEventHandler&) = delete;
	BasicalEventHandler& operator=(const BasicalEventHandler& event) = delete;
public:
	BasicalEventHandler() {};
	virtual ~BasicalEventHandler() {};
	//����()��Ϊ�˷���֮�����
	virtual void operator()(Args&...args) = 0;
};
//���ȶ����һ�������ӿڣ������Ϊ�˸�lambda,�º���ʹ�õĽӿڣ�Ϊ�˶�̬����lambda�ڴ�
//ͨ��new�����俽�����캯����������lambda���沶��Ĳ���������
template<typename func, typename ...Args>
class CommonEventHandler :public BasicalEventHandler<Args...>
{
private:
	func* m_handler;
public:
	CommonEventHandler(func f)
	{
		m_handler = new func(f);
	};
	void operator()(Args&...args) 
	{
		(*m_handler)(args...);
	} 
	~CommonEventHandler()
	{
		delete m_handler;
	}
};
//����ȫ�ֺ����ӿ�,��ʵ��ȫ�ֺ���Ҳ����ʹ�õ�һ���ӿ�
//����û��Ҫnewһ������,����Ϊ��ר�Ŷ���һ���ӿ�
template<typename func, typename ...Args>
class GlobalEventHandler :public BasicalEventHandler<Args...>
{
private:
	func m_handler;
public:
	GlobalEventHandler(func handler)
	{
		m_handler = handler;
	};
	void operator()(Args&...args) 
	{
		(*m_handler)(args...);
	}
};
//�����������ڲ�������,�����Ҫ��һ��������������ָ��
//���ڲ������Ľӿ�
template<typename T, typename ...Args>
class ClassEventHandler :public BasicalEventHandler<Args...>
{
	using func = void(T::*)(Args...);
private:
	T* m_receiver;
	func m_handler;
public:
	ClassEventHandler(T* receiver, func handler)
	{
		m_receiver = receiver;
		m_handler = handler;
	}
	void operator()(Args&...args) 
	{
		(m_receiver->*m_handler)(args...);
	}
};
