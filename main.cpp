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
int errorBound = int(1e9 + 7); //错误上限

void dfsPT(Type* now); //遍历普通语法分析树
void outputErrors();

int main(int argc, char** argv) {
	const string inFile = "code.pas"; //默认输入文件名
	const string outFile = "code.c"; //默认输出文件名

	FILE* fp;
	errno_t err = fopen_s(&fp, inFile.c_str(), "r");
	if (err) return -1;
	yyin = fp;
	yyparse(); //调用语法分析程序
	fclose(fp);

	cout << "Lexical Analysed" << endl;
	cout << "Syntax Analysed" << endl;

	//dfsPT(ParseTreeHead);

	if (!lexicalError.empty() || !syntaxError.empty()) //如果有词法、语法错误
	{
		outputErrors();
		return 0;
	}

	//语义分析
	_Program* ASTRoot = getProgram(ParseTreeHead);
	SemanticAnalyse(ASTRoot);//语义分析
	
	if (!semanticError.empty()) //如果有语义错误
	{
		outputErrors();
		return 0;
	}
	cout << "Semantic Analysed" << endl;

	//代码生成
	codeGenerate(ASTRoot, outFile);
	cout << "Code Generated" << endl;

	return 0;
}

//普通语法树的debug信息（遍历输出）
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