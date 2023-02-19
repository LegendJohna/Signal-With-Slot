#include "Event.h"
//示例代码信号与槽
//槽函数参数必须和信号参数对等 类型数量都一样
class Button
{
private:
	int data;
public:
	void setData(int num) {
		std::cout << "类内函数，参数是： " << num << std::endl;
		data = num;
	}
	void printData() { std::cout << "成员参数是：" << data << std::endl; };
	Event<int> clicked;
};

void print(int num)
{
	std::cout << "全局函数，参数是：  " << num << std::endl;
}
int main()
{
	int b = 10;
	Button* button = new Button();
	Button* button1 = new Button();
	button->clicked.connect([=](int num) {std::cout << "lambda表达式，参数是：" << num << std::endl; });
	button->clicked.emit(100);
	button->clicked.disconnect([=](int num) {std::cout << "lambda表达式，参数是：" << num << std::endl; });
	button->clicked.emit(100);

	std::cout << std::endl << std::endl << std::endl << "//////////////////////////////////" << std::endl << std::endl << std::endl;


	button1->printData();
	button->clicked.connect(button1, &Button::setData);
	button->clicked.emit(100);
	button1->printData();
	button->clicked.emit(100);
	//断开连接
	button->clicked.disconnect(button1, &Button::setData);
	button->clicked.emit(111);
	button1->printData();

	std::cout  << std::endl << std::endl << std::endl << "//////////////////////////////////" << std::endl << std::endl << std::endl;


	button->clicked.connect(&print);
	button->clicked.emit(111);
	button->clicked.disconnect(&print);
	button->clicked.emit(111);


	delete button;
	
}