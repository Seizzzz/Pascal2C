#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H
#include <string>
#include <vector>
#include <map>
using namespace std;

class _SymbolTable;

class _SymbolRecord {
public:
	string flag;//"value parameter"��ʾ��ֵ����,"var parameter"��ʾ�����ò���,"normal variant"��ʾ��ͨ����,"constant"��ʾ����,
	//"array"��ʾ����,"procedure"��ʾ����,"function"��ʾ����
	//"(sub)program name"��ʾ������¼�ǵ�ǰ���ű��Ӧ�ĳ�������Ϣ
	//"parameter of program"��ʾ������Ĳ���
	string id;
	int lineNumber;//����λ�õ��к�
	string type;//����Ǳ���/���������ʾ����/�������ͣ�
	//��������飬���ʾ����Ԫ�ص����ͣ�
	//����Ǻ��������ʾ��������ֵ���ͣ����ͱ���ֻ��Ϊ��������,"integer","real","char","boolean"
	string value;//����ǳ��������ʾ����ȡֵ
	bool isMinusShow;//����ǰ�Ƿ������
	//����ȡֵ��"integer","real","char","boolean"����
	int amount;//����ά��/����������
	//��������飬���ʾ����ά��������Ǻ���/���̣����ʾ��������
	vector<pair<int, int>> arrayRangeList;//�����ά���½�
	_SymbolTable* subSymbolTable;//ָ�����/������Ӧ���ַ��ű��ָ��

	string subprogramType;//����һ������ļ�¼����ʾ��ǰ���ű��Ӧ�ĳ������Ƶ���Ϣ���ñ�����ʾ�����Ǻ������ǹ���

	void setParam(string id, int lineNumber, string type);
	void setVariantParam(string id, int lineNumber, string type);
	void setVariant(string id, int lineNumber, string type);
	void setConst(string id, int lineNumber, string type, bool isMinusShow, string value);
	void setArray(string id, int lineNumber, string type, int amount, vector< pair<int, int> > arrayRangeList);
	void setProcedure(string id, int lineNumber, int amount, _SymbolTable* subSymbolTable);
	void setFunction(string id, int lineNumber, string type, int amount, _SymbolTable* subSymbolTable);
	void setPrgmName(string id, int lineNumber, string subprogramType, int amount, string returnType);
	void setVoidParam(string id, int lineNumber);

	string getIdxParamType(int X);//�ҵ���X����ʽ����������
	bool isIdxParamRef(int X);//����X����ʽ�����Ƿ������õ���
	bool isIdxOutRange(int X, int index);//����Xά�±��Ƿ�Խ�磬true��ʾԽ�磬false��ʾδԽ��

	_SymbolRecord() {
		arrayRangeList.clear();
		subSymbolTable = NULL;
	}
	~_SymbolRecord() {}
};

class _SymbolTable {
public:
	string tableType;//"main"����"sub"

	vector<_SymbolRecord*> recordList;
	map<string, int> idToLoc;//�ӿ��ѯ�ٶȵ�map
	map<string, int> idCount;//��ʶ������Ĵ���-1�����ڻָ���������ض���Ĵ���
	//ÿ����һ�����ţ���Ҫ��¼�����map��
	//ÿ��ѯһ�����ţ�������ͨ�����map��ѯ�÷�����vector�е�λ��

	void addParam(string id, int lineNumber, string type);
	void addVariantParam(string id, int lineNumber, string type);
	void addVariant(string id, int lineNumber, string type);
	void addConst(string id, int lineNumber, string type, bool isMinusShow, string value);
	void addArray(string id, int lineNumber, string type, int amount, vector<pair<int, int>> arrayRangeList);
	void addProcedure(string id, int lineNumber, int amount, _SymbolTable* subSymbolTable = NULL);
	void addFunction(string id, int lineNumber, string type, int amount, _SymbolTable* subSymbolTable = NULL);
	void addSubSymbolTable(string id, _SymbolTable* subSymbolTable);
	void addPrgmName(string id, int lineNumber, string subprogramType, int amount, string returnType);
	void addVoidParam(string id, int lineNumber);

	_SymbolTable(string type = "sub");
	~_SymbolTable() {}
};

#endif
