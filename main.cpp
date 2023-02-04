#include "Event.h"
//ʾ�������ź����
//�ۺ�������������źŲ����Ե� ����������һ��
class Button
{
private:
	int data = 0;
public:
	void setData(int num) { data = num; }
	void printData() { std::cout << "���ں���,������" << data << std::endl; };
	Event<int> clicked;
};

void print(int num)
{
	std::cout << "��ͨ����" << " �����ǣ�"  <<num<< std::endl;
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