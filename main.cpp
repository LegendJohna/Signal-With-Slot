#include "Event.h"
//示例代码信号与槽
//槽函数参数必须和信号参数对等 类型数量都一样
class Button
{
private:
	int data = 0;
public:
	void setData(int num) { data = num; }
	void printData() { std::cout << "类内函数,参数是" << data << std::endl; };
	Event<int> clicked;
};

void print(int num)
{
	std::cout << "普通函数" << " 参数是："  <<num<< std::endl;
}

int main()
{
	Button* button = new Button();;
	button->clicked.connect(&print);
	int lambdaConnection = button->clicked.connect([=](int num) {std::cout << num << std::endl; });
	button->clicked.connect(button,&Button::setData);
	button->clicked.emit(1);
	button->clicked.disconncet(lambdaConnection);
	button->clicked.emit(10);
	button->clicked.disconncet("ALL");
	button->clicked.emit(1);
	delete button;
}