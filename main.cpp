#include "PTnode.h"
#include <fstream>

class _Program;

extern Type* ParseTreeHead;
extern FILE* yyin;
extern _Program* getProgram(Type* now);
extern int yydebug;
extern void SemanticAnalyse(_Program* ASTRoot);
extern vector<string> lexicalError;
extern vector<string> syntaxError;
extern vector<string> semanticError;
extern void codeGenerate(_Program* ASTRoot, string outName);

extern "C" {
	int yyparse();
}

int errorCount = 0;
int errorBound = int(1e9 + 7); //��������

void dfsPT(Type* now); //������ͨ�﷨������
void outputErrors();

int main(int argc, char** argv) {
	const string inFile = "code.pas"; //Ĭ�������ļ���
	const string outFile = "code.c"; //Ĭ������ļ���

	FILE* fp;
	errno_t err = fopen_s(&fp, inFile.c_str(), "r");
	if (err) return -1;
	yyin = fp;
	yyparse(); //�����﷨��������
	fclose(fp);

	cout << "Lexical Analysed" << endl;
	cout << "Syntax Analysed" << endl;

	//dfsPT(ParseTreeHead);

	if (!lexicalError.empty() || !syntaxError.empty()) //����дʷ����﷨����
	{
		outputErrors();
		return 0;
	}

	//�������
	_Program* ASTRoot = getProgram(ParseTreeHead);
	SemanticAnalyse(ASTRoot);//�������
	
	if (!semanticError.empty()) //������������
	{
		outputErrors();
		return 0;
	}
	cout << "Semantic Analysed" << endl;

	//��������
	codeGenerate(ASTRoot, outFile);
	cout << "Code Generated" << endl;

	return 0;
}

//��ͨ�﷨����debug��Ϣ�����������
void dfsPT(Type* now)
{
	if (now->children.empty())
	{
		if (now->str.empty())
			cout << now->token << " -> empty" << endl;
		return;
	}
	cout << now->token << " ->";

	for (auto* child : now->children)
	{
		if (child->children.empty() && !child->str.empty())
			cout << " \"" << child->str << "\"";
		else
			cout << " " << child->token;
	}
	cout << endl;

	for (auto* child : now->children)
		dfsPT(child);
}

void outputErrors() {
	if (!lexicalError.empty())
		for (auto info : lexicalError)
			cout << info << endl;

	if (!syntaxError.empty())
		for (auto info : syntaxError)
			cout << info << endl;

	if (!semanticError.empty())
		for (auto info : semanticError)
			cout << info << endl;
}