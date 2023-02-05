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
	Event<> clicked;
};

void print(int num)
{
	std::cout << "普通函数" << " 参数是："  <<num<< std::endl;
}

int main()
{
	int b = 10;
	Button* button = new Button();
	button->clicked.connect(button, &Button::printData);
	button->clicked.emit();
	button->clicked.emit();
	button->clicked.disconncet("ALL");
	button->clicked.emit();
	delete button;
}