#pragma once
/*
	这个类是在定义抽象函数接口
	为了实现类似function那样的功能
	采用的方法是多态+模板
*/
//可变模板参数
template <typename ...Args>
class BasicalEventHandler
{
private:
	BasicalEventHandler(const BasicalEventHandler&) = delete;
	BasicalEventHandler& operator=(const BasicalEventHandler& event) = delete;
public:
	BasicalEventHandler() {};
	virtual ~BasicalEventHandler() {};
	//重载()是为了方便之后调用
	virtual void operator()(Args&...args) = 0;
};
//首先定义第一个函数接口，这个是为了给lambda,仿函数使用的接口，为了动态管理lambda内存
//通过new调用其拷贝构造函数，否则在lambda里面捕获的参数会销毁
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
//定义全局函数接口,事实上全局函数也可以使用第一个接口
//但是没必要new一个拷贝,我们为它专门定义一个接口
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
//接下来是类内部函数啦,这个需要多一个变量来存对象的指针
//类内部函数的接口
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
