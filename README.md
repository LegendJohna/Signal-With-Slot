# Signal-With-Slot

 ## 观察者模式
 观察者模式是一种著名的设计模式,观察者可以订阅某些事件，在事件发生的时候会通知订阅事件的观察者,如下有一个简单的实现
 
 事件实现
 ```c++
#include <list>
#include <string>
using std::list;
using std::string;
class Observer;
class Subject
{
protected:
		list<Observer*> ObserverList;
public:
		void attach(Observer* observer)
		{
			ObserverList.push_back(observer);
		}
		void deatch(Observer* observer)
		{
			ObserverList.remove(observer);
		}
		virtual void notify(string msg) = 0;
		virtual ~Subject() {};
};
```
观察者实现
```c++
#include "Subject.h"
class Observer
{
protected:
	string m_Name;
public:
	Observer(string name):m_Name(name){}
	void subscribe(Subject * subject)
	{
		subject->attach(this);
	}
	void unsubscribe(Subject* subject)
	{
		subject->deatch(this);
	}
	virtual void update(string msg) = 0;
	virtual ~Observer() {}
};
```
只需要继承这两了接口,便可以实现事件的订阅与发布,但显然这样写的缺点还是很大的,不灵活，还有耦合性高

 ## 信号和槽
 信号和槽是qt基于moc创建的一套事件机制,是上面观察者模式的一种实现,用起来十分好用,但是用原生c++开发的时候就没法用这么好的特性了,所以想要在原版c++里面复刻一个类似的机制来使用,幸运的是c++模板机制的强大已经可以做到这一点

来看我们实现的一些例子吧:
```c++
class Button :public Object
{
private:
	int data = 0;
public:
	Event<int> clicked;
	void setData(int num) 
	{
		data = num;
	}
	void printData() { std::cout << "成员参数是：" << data << std::endl; };
};
```
我们自定义了一个Button类,你可以发现它继承了Object这个类,如果你想使用信号槽,那你就必须继承这个类,作用我们后面再说,你还可以发现,我们定义一个Event<int>类型的变量,clicked,这个int是指你所要传递的参数类型,

接下来让我们看看如何订阅事件吧
```c++
int main()
{
	Button* button = new Button();
	Button* button1 = new Button();
	button->clicked.connect(button1, &Button::setData);
	button->clicked.emit(100);
	button1->printData();
	delete button;
	delete button1;
	return 0;
}
```
运行一下程序,你会发现button1里面的data已经被修改了,使用起来就是这么简单,只要将接收者的地址,和成员函数指针,传入就行了,值得注意的是,函数的参数
必须与Event<>中的完全一致,包括顺序,这样它才能正常工作

同时我们也支持断开连接的操作
```c++
int main()
{
	Button* button = new Button();
	Button* button1 = new Button();
	button->clicked.connect(button1, &Button::setData);
	button->clicked.emit(100);
	button1->printData();
	button->clicked.disconnect(button1, &Button::setData);
	button->clicked.emit(120);
	button1->printData();
	delete button;
	delete button1;
	return 0;
}
```
	
你会发现第二次打印出的值仍然是100,而不是后来设定的120,是不是用起来很自由呢，同时你不仅仅可以绑定类的成员函数,你还可以绑定全局函数,lambda表达式,静态函数,仿函数,如下
```c++
void print(int num)
{
	std::cout << "全局函数，参数是：  " << num << std::endl;
}
int main()
{
	Button* button = new Button();
	button->clicked.connect([=](int num) {std::cout << "lambda表达式，参数是：" << num << std::endl; });
	button->clicked.connect(&print);
	button->clicked.emit(100);
	delete button;
	return 0;
}
```
## 安全性
虽然，我们可以灵活的手动disconnect来断开连接,可是万一我忘记断开连接,并且接收者对象析构,这时程序会不会出问题呢？就像下面这种情况
```c++	
int main()
{
	Button* button = new Button();
	Button* button1 = new Button();
	button->clicked.connect(button1, &Button::setData);
	delete button1;
	button->clicked.emit(100);
	delete button;
	return 0;
}
```
还记得你继承的Object吗,事实上正得益于它,在对象销毁的时候会自动断开,所有与之相关的信号,从而减少使用者的心智负担,并且还支持线程安全,所以你可以自由的使用信号槽而不用有任何担心啦
