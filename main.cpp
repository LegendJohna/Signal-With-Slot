#include "Event.h"
//ʾ�������ź����
//�ۺ�������������źŲ����Ե� ����������һ��
class Button
{
private:
	int data;
public:
	void setData(int num) {
		std::cout << "���ں����������ǣ� " << num << std::endl;
		data = num;
	}
	void printData() { std::cout << "��Ա�����ǣ�" << data << std::endl; };
	Event<int> clicked;
};

void print(int num)
{
	std::cout << "ȫ�ֺ����������ǣ�  " << num << std::endl;
}
int main()
{
	int b = 10;
	Button* button = new Button();
	Button* button1 = new Button();
	button->clicked.connect([=](int num) {std::cout << "lambda���ʽ�������ǣ�" << num << std::endl; });
	button->clicked.emit(100);
	button->clicked.disconnect([=](int num) {std::cout << "lambda���ʽ�������ǣ�" << num << std::endl; });
	button->clicked.emit(100);

	std::cout << std::endl << std::endl << std::endl << "//////////////////////////////////" << std::endl << std::endl << std::endl;


	button1->printData();
	button->clicked.connect(button1, &Button::setData);
	button->clicked.emit(100);
	button1->printData();
	button->clicked.emit(100);
	//�Ͽ�����
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