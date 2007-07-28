#include <iostream>

using namespace std;

int main (int argc, char** argv)
{
	string expression;

	cout << "Insert expression to parse" << endl;
	getline(cin, expression);
	cout << expression << endl;

	return 0;
}
