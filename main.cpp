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
	Event<> clicked;
};

void print(int num)
{
	std::cout << "��ͨ����" << " �����ǣ�"  <<num<< std::endl;
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