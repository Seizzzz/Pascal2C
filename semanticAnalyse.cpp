#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "symbolTable.h"
#include "PTnode.h"
#include "ASTnode.h"

extern void outputErrors();
extern int errorCount;
extern int errorBound;
#define CHECK_ERROR_BOUND errorCount++;\
if(errorCount>=errorBound){\
    cout << "There have been more than " << errorBound << " errors, compiler abort." << endl;\
    outputErrors();\
    exit(0);\
}

using namespace std;

extern _SymbolTable* mainSymbolTable; 
extern _SymbolTable* currSymbolTable; 
extern _SymbolRecord* findSymbolRecord(_SymbolTable* currSymbolTable, string id, int mode = 0);
extern void getFunctionCall(_FunctionCall* functionCallNode, string& functionCall, int mode = 0);
extern int getExpression(_Expression* expressionNode, string& expression, int mode = 0, bool isReferedActualPara = false);
extern void getVariantRef(_VariantReference* variantRefNode, string& variantRef, int mode = 0, bool isReferedActualPara = false);//获取变量引用
extern int str2int(string str);

vector<string> semanticError; //存储错误信息的容器

void SemanticAnalyse(_Program* ASTRoot);

void newMainSymbolTable();//创建主符号表并初始化
void newSubSymbolTable();//创建子符号表并初始化

string analyseVariantRef(_VariantReference* variantReference); 
void analyseStatement(_Statement* statement); 
void analyseSubPrgmDefine(_FunctionDefinition* functionDefinition); 
void analyseVariant(_Variant* variant); 
void analyseConst(_Constant* constant); 
void analyseSubPrgm(_SubProgram* subprogram); 
void analysePrgm(_Program* program); //对主程序进行语义分析
void analyseFormalParam(_FormalParameter* formalParameter); 
string analyseFunctionCall(_FunctionCall* functionCall); 
string analyseExpression(_Expression* expression); 
bool isIdIllgal(string id, int lineNumber); //检查标识符是否与库程序名、主程序名、主程序参数同名

string itos(int num);//将int转化为string

void addDuplicateDefinitionError(string preId, int preLineNumber, string preFlag, string preType, int curLineNumber); //标识符重定义
void addUndefinedError(string id, int curLineNumber); //标识符未定义
void addUsageTypeError(string curId, int curLineNumber, string curType, string usage, string correctType); //标识符类型(type)错误
void addNumberError(string curId, int curLineNumber, int curNumber, int correctNumber, string usage); //数组维数、过程或函数参数个数不匹配
void addPreFlagError(string curId, int curLineNumber, string curFlag, int preLineNumber, string preFlag); //标识符种类(flag)错误
void addExpressionTypeError(_Expression* exp, string curType, string correctType, string description); //表达式类型错误
void addAssignTypeMismatchError(_VariantReference* leftVariantReference, _Expression* rightExpression); //赋值语句左值和右值类型不匹配
void addArrayRangeOutOfBoundError(_Expression* expression, string arrayId, int X, pair<int, int> range); //数组下标越界
void addArrayRangeUpSideDownError(string curId, int curLineNumber, int X, int lowBound, int highBound); //数组上下界不合法
void addOperandExpressionsTypeMismatchError(_Expression* exp1, _Expression* exp2); //双目运算符左右类型不匹配
void addSingleOperandExpressionTypeMismatchError(_Expression* exp, string correctType); //操作数无合法的单目运算结果
void addactualParameterOfReadError(int curLineNumber, string procedureId, int X, _Expression* exp); //read实参错误
void addDivideZeroError(string operation, _Expression* exp); //除法右操作数为0
void addReadBooleanError(_Expression* exp, int X); //添加read读取boolean类型变量错误的信息
void addGeneralError(string error); //其他错误信息

void SemanticAnalyse(_Program* ASTRoot) {
	newMainSymbolTable();
	analysePrgm(ASTRoot);
}

void newMainSymbolTable() { //创建主符号表
	currSymbolTable = mainSymbolTable = new _SymbolTable("main");//指定为主符号表后，会自动加入read, write等库函数
}

void newSubSymbolTable() {//创建子符号表 符号表定位
	currSymbolTable = new _SymbolTable("sub");//创建并定位到子符号表
}

//对变量引用进行语义分析
//变量引用作为左值，可能是传值参数、引用参数、普通变量、数组元素、函数名（函数返回值）
//变量引用作为右值，可能是传值参数、引用参数、普通变量、数组元素、函数名（不带参数的函数）
string analyseVariantRef(_VariantReference* variantReference) {
	if (variantReference == NULL) {
		cout << "[analyseVariantRef] pointer of _VariantReference is null" << endl;
		return "";
	}
	_SymbolRecord* record = findSymbolRecord(currSymbolTable, variantReference->variantId.first);
	if (record == NULL) {   
		addUndefinedError(variantReference->variantId.first, variantReference->variantId.second);
		return variantReference->variantType = "error";//无法找到变量引用的类型
	}
	if (variantReference->flag == 0) {//如果是非数组
		if (record->flag == "(sub)program name") {
			if (record->subprogramType == "procedure") {
				addGeneralError("[Invalid reference] <Line " + itos(variantReference->variantId.second) + "> Procedure name \"" + record->id + "\" can't be referenced");
				return variantReference->variantType = "error";
			}
			
			if (variantReference->locFlag == -1) { //-1代表是左值
				variantReference->kind = "function return reference";
				return variantReference->variantType = record->type;
			}
			if (record->amount != 0) {//如果形参个数不为0    递归调用
				addNumberError(variantReference->variantId.first, variantReference->variantId.second, 0, record->amount, "function");
				return variantReference->variantType = record->type;
			}
			//如果形参个数为0
			//这样对应到具体程序中，实际上是无参函数的递归调用
			variantReference->kind = "function call";
			return variantReference->variantType = record->type;
		}
		if (record->flag == "function") {//如果是函数 则一定是在主符号表中查到的
			variantReference->kind = "function";
			//不能作为左值，必须作为右值，且形参个数必须为0
			//被识别为variantReference的函数调用一定不含实参，所以需要检查形参个数
			if (variantReference->locFlag == -1) {//如果是左值   
				addGeneralError("[Invalid reference!] <Line " + itos(variantReference->variantId.second) + "> function name \"" + record->id + "\" can't be referenced as l-value.");
				return variantReference->variantType = "error";
			}
			//如果是右值
			if (record->amount != 0) {//如果形参个数不为0   
				addNumberError(variantReference->variantId.first, variantReference->variantId.second, 0, record->amount, "function");
				return variantReference->variantType = record->type;
			}
			return variantReference->variantType = record->type;
		}
		 
		if (!(record->flag == "value parameter" || record->flag == "var parameter" || record->flag == "normal variant" || record->flag == "constant")) {
			addGeneralError("[Invalid reference!] <Line " + itos(variantReference->variantId.second) + "> \"" + variantReference->variantId.first + "\" is a " + record->flag + ", it can't be referenced.");
			return variantReference->variantType = "error";
		}
		variantReference->kind = "var";
		if (record->flag == "constant")
			variantReference->kind = "constant";
		return variantReference->variantType = record->type;
	}
	else if (variantReference->flag == 1) {//flag是1代表是数组
		if (record->flag != "array") {//如果符号表中记录的不是数组 
			addPreFlagError(variantReference->variantId.first, variantReference->variantId.second, "array", record->lineNumber, record->flag);
			return variantReference->variantType = "error";
		}
		variantReference->kind = "array";
		 
		if (variantReference->expressionList.size() != record->amount) {//如果引用时的下标维数和符号表所存不一致
			addNumberError(variantReference->variantId.first, variantReference->variantId.second, int(variantReference->expressionList.size()), record->amount, "array");
			variantReference->variantType = "error";
			return record->type;
		}
		variantReference->variantType = record->type;
		for (int i = 0; i < variantReference->expressionList.size(); i++) {
			string type = analyseExpression(variantReference->expressionList[i]);
			//检查下标表达式的类型是否是整型   
			if (type != "integer") {
				addExpressionTypeError(variantReference->expressionList[i], type, "integer", itos(i + 1) + "th index of array \"" + variantReference->variantId.first + "\"");
				variantReference->variantType = "error";
			}
			//检查越界   
			if (variantReference->expressionList[i]->totalIntValueValid) {
				if (!record->isIdxOutRange(i, variantReference->expressionList[i]->totalIntValue)) {
					addArrayRangeOutOfBoundError(variantReference->expressionList[i], variantReference->variantId.first, i, record->arrayRangeList[i]);
					variantReference->variantType = "error";
				}
			}
		}
		return record->type;
	}
	else {
		cout << "[analyseVariantRef] flag of variantReference is not 0 or 1" << endl;
		return variantReference->variantType = "error";
	}
}

void analyseStatement(_Statement* statement) {
	if (statement == NULL) {
		cout << "[analyseStatement] pointer of _Statement is null" << endl;
		return;
	}
	if (statement->type == "compound")
	{
		_Compound* compound = reinterpret_cast<_Compound*>(statement);
		for (auto iter : compound->statementList)
			analyseStatement(iter);
	}
	else if (statement->type == "repeat") {
		_RepeatStatement* repeatStatement = reinterpret_cast<_RepeatStatement*>(statement);
		string type = analyseExpression(repeatStatement->condition);
		if (type != "boolean") {//repeat语句类型检查,condition表达式类型检查   
			addExpressionTypeError(repeatStatement->condition, type, "boolean", "condition of repeat-until statement");
			repeatStatement->statementType = "error";
		}
		else
			repeatStatement->statementType = "void";
		analyseStatement(repeatStatement->_do);
	}
	else if (statement->type == "while") {
		_WhileStatement* whileStatement = reinterpret_cast<_WhileStatement*>(statement);
		string type = analyseExpression(whileStatement->condition);
		if (type != "boolean") {//while语句类型检查,condition表达式类型检查   
			addExpressionTypeError(whileStatement->condition, type, "boolean", "condition of while statement");
			whileStatement->statementType = "error";
		}
		else
			whileStatement->statementType = "void";
		analyseStatement(whileStatement->_do);
	}
	else if (statement->type == "for") {
		_ForStatement* forStatement = reinterpret_cast<_ForStatement*>(statement);
		//检查循环变量是否已经定义，如已经定义，是否为integer型变量
		_SymbolRecord* record = findSymbolRecord(currSymbolTable, forStatement->id.first);
		if (record == NULL) {//循环变量未定义，错误信息   
			addUndefinedError(forStatement->id.first, forStatement->id.second);
			return;
		}
		//如果无法作为循环变量   
		if (!(record->flag == "value parameter" || record->flag == "var parameter" || record->flag == "normal variant")) {//如果当前符号种类不可能作为循环变量
			addPreFlagError(forStatement->id.first, forStatement->id.second, "value parameter, var parameter or normal variant", record->lineNumber, record->flag);
			return;
		}
		if (record->type != "integer") {
			addUsageTypeError(forStatement->id.first, forStatement->id.second, record->type, "cyclic variable of for statement", "integer");
			return;
		}
		forStatement->statementType = "void";
		string type = analyseExpression(forStatement->start);
		if (type != "integer") {  
			addExpressionTypeError(forStatement->start, type, "integer", "start value of for statement");
			forStatement->statementType = "error";
		}
		type = analyseExpression(forStatement->end);
		if (type != "integer") {  
			addExpressionTypeError(forStatement->end, type, "integer", "end value of for statement");
			forStatement->statementType = "error";
		}
		//对循环体语句进行语义分析
		analyseStatement(forStatement->_do);
	}
	else if (statement->type == "if") {
		_IfStatement* ifStatement = reinterpret_cast<_IfStatement*>(statement);
		string type = analyseExpression(ifStatement->condition);
		if (type != "boolean") {//if语句类型检查,condition表达式类型检查   
			addExpressionTypeError(ifStatement->condition, type, "boolean", "condition of if statement");
			ifStatement->statementType = "error";
		}
		else
			ifStatement->statementType = "void";
		analyseStatement(ifStatement->then);//对then语句进行语义分析
		if (ifStatement->els != NULL)//对else语句进行语句分析
			analyseStatement(ifStatement->els);
	}
	else if (statement->type == "assign") {//左值特判
		_AssignStatement* assignStatement = reinterpret_cast<_AssignStatement*>(statement);
		//对左值变量引用进行语义分析,获得leftType
		assignStatement->statementType = "void";
		assignStatement->variantReference->locFlag = -1;//标记为左值
		string leftType = analyseVariantRef(assignStatement->variantReference);
		if (assignStatement->variantReference->kind == "constant") {
			//左值不能为常量   
			addGeneralError("[Constant as l-value error!] <Line" + itos(assignStatement->variantReference->variantId.second) + "> Costant \"" + assignStatement->variantReference->variantId.first + "\" can't be referenced as l-value.");
			return;
		}
		string rightType = analyseExpression(assignStatement->expression);
		if (assignStatement->variantReference->kind == "function return reference") {
			if (assignStatement->variantReference->variantType != rightType && !(assignStatement->variantReference->variantType == "real" && rightType == "integer")) {
				 
				addGeneralError("[Return type of funciton mismatch!] <Line " + itos(assignStatement->expression->lineNumber) + "> The type of return expression is " + rightType + " ,but not " + assignStatement->variantReference->variantType + " as function \"" + assignStatement->variantReference->variantId.first + "\" defined.");
				assignStatement->statementType = "error";
			}
			assignStatement->isReturnStatement = true;
			return;
		}
		//比较左值和右值类型,获得赋值语句的类型；类型不同时，只支持整型到实型的隐式转换
		if (leftType != rightType && !(leftType == "real" && rightType == "integer")) {
			 
			addAssignTypeMismatchError(assignStatement->variantReference, assignStatement->expression);
			assignStatement->statementType = "error";
		}
		else
			assignStatement->statementType = "void";
	}
	else if (statement->type == "procedure") {//read的参数只能是变量或数组元素;
		_ProcedureCall* procedureCall = reinterpret_cast<_ProcedureCall*>(statement);
		_SymbolRecord* record = findSymbolRecord(mainSymbolTable, procedureCall->procedureId.first);
		if (record == NULL)
			record = findSymbolRecord(currSymbolTable, procedureCall->procedureId.first, 1);
		procedureCall->statementType = "void";
		if (record == NULL) {    
			addUndefinedError(procedureCall->procedureId.first, procedureCall->procedureId.second);
			procedureCall->statementType = "error";
			return;
		}
		if (record->flag != "procedure") {//如果不是过程   
			addPreFlagError(procedureCall->procedureId.first, procedureCall->procedureId.second, "procedure", record->lineNumber, record->flag);
			procedureCall->statementType = "error";
			return;
		}
		if (record->id == "exit") {
			/*exit既可以出现在过程中，也可以出现在函数中，出现在过程中时，
			exit不能带参数，出现在函数中时，exit只能带一个参数，
			且该参数表达式的类型必须和函数的返回值类型一致*/
			//所以需判断当前程序是过程还是函数
			if (currSymbolTable->recordList[0]->subprogramType == "procedure") {//如果是过程
				//exit不能带参数表达式
				if (procedureCall->actualParaList.size() != 0) {//如果实参个数不为0   
					addGeneralError("[Return value redundancy!] <Line " + itos(procedureCall->procedureId.second) + "> Number of return value of procedure must be 0, that is, exit must have no actual parameters.");
					procedureCall->statementType = "error";
				}
				return;
			}
			//如果是函数
			if (procedureCall->actualParaList.size() != 1) {//如果实参个数不为1
				if (procedureCall->actualParaList.size() == 0)  
					addGeneralError("[Return value missing!] <Line " + itos(procedureCall->procedureId.second) + "> Number of return value of function must be 1, that is, exit must have 1 actual parameters.");
				else  
					addGeneralError("[Return value redundancy!] <Line " + itos(procedureCall->procedureId.second) + "> Number of return value of function must be 1, that is, exit must have 1 actual parameters.");
				return;
			}
			//如果实参个数为1，检查实参表达式的类型，检查是否与函数返回值类型一致
			string returnType = analyseExpression(procedureCall->actualParaList[0]);
			if (currSymbolTable->recordList[0]->type != returnType && !(currSymbolTable->recordList[0]->type == "real" && returnType == "integer")) {
				 
				addGeneralError("[Return type of funciton mismatch!] <Line " + itos(procedureCall->actualParaList[0]->lineNumber) + "> The type of return expression is " + returnType + " ,but not " + currSymbolTable->recordList[0]->type + " as function \"" + currSymbolTable->recordList[0]->id + "\" defined.");
				procedureCall->statementType = "error";
			}
			procedureCall->isReturnStatement = true;
			return;
		}
		if (record->id == "read" || record->id == "write") {
			if (procedureCall->actualParaList.size() == 0) { //read、write的参数个数不能为0   
				string tmp = record->id;
				tmp[0] -= 'a' - 'A';
				addGeneralError("[" + tmp + " actual parameter missing!] <Line " + itos(procedureCall->procedureId.second) + "> procedure \"" + record->id + "\" must have at least one actual parameter.");
				procedureCall->statementType = "error";
			}
		}
		if (record->id == "read") {//参数只能是变量或数组元素，不能是常量、表达式等
			for (int i = 0; i < procedureCall->actualParaList.size(); i++) {
				string actualType = analyseExpression(procedureCall->actualParaList[i]);
				if (!(procedureCall->actualParaList[i]->type == "var" && (procedureCall->actualParaList[i]->variantReference->kind == "var" || procedureCall->actualParaList[i]->variantReference->kind == "array")))
					addactualParameterOfReadError(procedureCall->actualParaList[i]->lineNumber, record->id, i + 1, procedureCall->actualParaList[i]);
				if (procedureCall->actualParaList[i]->expressionType == "boolean")
					addReadBooleanError(procedureCall->actualParaList[i], i + 1);
				if (actualType == "error")
					procedureCall->statementType = "error";
			}
			return;
		}
		if (record->amount == -1) {//如果是变参过程（本编译器涉及的变参过程(除了read)对参数类型没有要求，但不能为error）
			for (auto iter : procedureCall->actualParaList)
			{
				string actualType = analyseExpression(iter);
				if (actualType == "error")
					procedureCall->statementType = "error";
			}
			return;
		}
		if (procedureCall->actualParaList.size() != record->amount) {  
			addNumberError(procedureCall->procedureId.first, procedureCall->procedureId.second, int(procedureCall->actualParaList.size()), record->amount, "procedure");
			procedureCall->statementType = "error";
			return;
		}
		// 形参在符号表中的定位
		for (int i = 0; i < procedureCall->actualParaList.size(); i++) {//检查actualParaList各表达式的类型，检查实参和形参的类型一致性
			string actualType = analyseExpression(procedureCall->actualParaList[i]);
			string formalType = record->getIdxParamType(i + 1);
			bool isRefered = record->isIdxParamRef(i + 1);
			if (isRefered && !(procedureCall->actualParaList[i]->type == "var" && (procedureCall->actualParaList[i]->variantReference->kind == "var" || procedureCall->actualParaList[i]->variantReference->kind == "array"))) {
				//该表达式不能作为引用形参对应的实参   
				addGeneralError("[Referenced actual parameter error!] <Line " + itos(procedureCall->actualParaList[i]->lineNumber) + "> The " + itos(i + 1) + "th actual parameter expression should be a normal variable、value parameter、referenced parameter or array element.");
				continue;
			}
			//if(isRefered && procedureCall->actualParaList[i]->type==)
			if (!isRefered) { //传值参数支持integer到real的隐式类型转换
				if (actualType != formalType && !(actualType == "integer" && formalType == "real")) { //如果类型不一致
					 
					addExpressionTypeError(procedureCall->actualParaList[i], actualType, formalType, itos(i + 1) + "th actual parameter of procedure call of \"" + procedureCall->procedureId.first + "\"");
					procedureCall->statementType = "error";
				}
			}
			else { //引用参数需保持类型强一致
				if (actualType != formalType) { //如果类型不一致
					 
					addExpressionTypeError(procedureCall->actualParaList[i], actualType, formalType, itos(i + 1) + "th actual parameter of procedure call of \"" + procedureCall->procedureId.first + "\"");
					procedureCall->statementType = "error";
				}
			}
		}
	}
	else {
		cout << "[analyseStatement] statement type error" << endl;
		return;
	}
}

//对形式参数进行语义分析，形式参数一定是基本类型
void analyseFormalParam(_FormalParameter* formalParameter) {
	if (formalParameter == NULL) {
		cout << "[analyseFormalParam] pointer of _FormalParameter is null" << endl;
		return;
	}
	//return;
	_SymbolRecord* record = findSymbolRecord(currSymbolTable, formalParameter->paraId.first, 1);
	if (!isIdIllgal(formalParameter->paraId.first, formalParameter->paraId.second)) {
		if (record != NULL) //如果重定义
			addDuplicateDefinitionError(record->id, record->lineNumber, record->flag, record->type, formalParameter->paraId.second);
	}
	//检查是否与当前程序名以及在这之前定义的形参同名，如果同名，添加下划线进行恢复（在add函数中进行）
	if (formalParameter->flag == 0)
		currSymbolTable->addParam(formalParameter->paraId.first, formalParameter->paraId.second, formalParameter->type);
	else
		currSymbolTable->addVariantParam(formalParameter->paraId.first, formalParameter->paraId.second, formalParameter->type);
}

//对子程序定义进行语义分析
void analyseSubPrgmDefine(_FunctionDefinition* functionDefinition) {
	if (functionDefinition == NULL) {
		cout << "[analyseSubPrgmDefine] pointer of _FunctionDefinition is null" << endl;
		return;
	}
	_SymbolRecord* record = findSymbolRecord(currSymbolTable, functionDefinition->functionID.first, 1);
	if (record != NULL) {//重定义   
		addDuplicateDefinitionError(record->id, record->lineNumber, record->flag, record->type, functionDefinition->functionID.second);
		return;
	}
	string subprogramType;
	if (functionDefinition->type.first.empty())
		subprogramType = "procedure";
	else
		subprogramType = "function";
	newSubSymbolTable(); //创建并定位到子符号表
	//将子程序名等信息添加到子符号表中
	currSymbolTable->addPrgmName(functionDefinition->functionID.first, functionDefinition->functionID.second, subprogramType, int(functionDefinition->formalParaList.size()), functionDefinition->type.first);

	//根据type是否为NULL，分为addProcedure()和addFunction()，添加到主程序表中
	if (functionDefinition->type.first.empty())//如果是过程
		mainSymbolTable->addProcedure(functionDefinition->functionID.first, functionDefinition->functionID.second, int(functionDefinition->formalParaList.size()), currSymbolTable);
	else//如果是函数
		mainSymbolTable->addFunction(functionDefinition->functionID.first, functionDefinition->functionID.second, functionDefinition->type.first, int(functionDefinition->formalParaList.size()), currSymbolTable);

	
	for (auto iter : functionDefinition->formalParaList)
		analyseFormalParam(iter);
	for (auto iter : functionDefinition->constList)
		analyseConst(iter);
	for (auto iter : functionDefinition->variantList)
		analyseVariant(iter);
	analyseStatement(reinterpret_cast<_Statement*>(functionDefinition->compound));
}

void analyseVariant(_Variant* variant) {
	if (variant == NULL) {
		cout << "[analyseVariant] pointer of _Variant is null" << endl;
		return;
	}
	_SymbolRecord* record = findSymbolRecord(currSymbolTable, variant->variantId.first, 1);//用variantId.first去查符号表，检查是否重定义
	if (isIdIllgal(variant->variantId.first, variant->variantId.second))
		return;
	if (record != NULL) { //如果重定义
		addDuplicateDefinitionError(record->id, record->lineNumber, record->flag, record->type, variant->variantId.second);
		return;
	}
	if (variant->type->flag == 0)//如果是普通变量
		currSymbolTable->addVariant(variant->variantId.first, variant->variantId.second, variant->type->type.first);
	else {//如果是数组
		//数组定义时，上下界的限制是上界必须大于等于下界；按照文法定义，上下界均为无符号数，且通过了语法分析，就一定是无符号数
		vector<pair<int, int>>& tmp = variant->type->arrayRangeList;
		for (int i = 0; i < tmp.size(); i++) {
			if (tmp[i].first > tmp[i].second) {  
				addArrayRangeUpSideDownError(variant->variantId.first, variant->variantId.second, i + 1, tmp[i].first, tmp[i].second);
				tmp[i].second = tmp[i].first; //如果上界小于下界，将上界设置为下界
			}
		}
		currSymbolTable->addArray(variant->variantId.first, variant->variantId.second, variant->type->type.first, int(variant->type->arrayRangeList.size()), variant->type->arrayRangeList);
	}
}

//对常量定义进行语义分析
void analyseConst(_Constant* constant) {
	if (constant == NULL) {
		cout << "[analyseConst] pointer of _Constant is null" << endl;
		return;
	}
	//用constId.first去查符号表，检查是否重定义 
	_SymbolRecord* record = findSymbolRecord(currSymbolTable, constant->constId.first, 1);
	if (isIdIllgal(constant->constId.first, constant->constId.second))
		return;
	if (record != NULL) {//重定义   
		addDuplicateDefinitionError(record->id, record->lineNumber, record->flag, record->type, constant->constId.second);
		return;
	}
	if (constant->type == "id") { //如果该常量由另外的常量标识符定义
		_SymbolRecord* preRecord = findSymbolRecord(currSymbolTable, constant->valueId.first);
		if (preRecord == NULL) {    
			addUndefinedError(constant->valueId.first, constant->valueId.second);
			return;
		}
		if (preRecord->flag != "constant") {//如果不是常量   
			addPreFlagError(constant->valueId.first, constant->valueId.second, "constant", preRecord->lineNumber, preRecord->flag);
			return;
		}
		currSymbolTable->addConst(constant->constId.first, constant->constId.second, preRecord->type, constant->isMinusShow ^ preRecord->isMinusShow, preRecord->value);
	}
	else//该常量由常数值定义
		currSymbolTable->addConst(constant->constId.first, constant->constId.second, constant->type, constant->isMinusShow, constant->strOfVal);
}

//对子程序进行语义分析
void analyseSubPrgm(_SubProgram* subprogram) {
	if (subprogram == NULL) {
		cout << "[analyseSubPrgm] pointer of _Subprogram is null" << endl;
		return;
	}
	for (int i = 0; i < subprogram->constList.size(); i++)
		analyseConst(subprogram->constList[i]);
	for (int i = 0; i < subprogram->variantList.size(); i++)
		analyseVariant(subprogram->variantList[i]);
	for (int i = 0; i < subprogram->subprogramDefinitionList.size(); i++) {
		analyseSubPrgmDefine(subprogram->subprogramDefinitionList[i]);
		currSymbolTable = mainSymbolTable;//符号表重定位
	}
	analyseStatement(reinterpret_cast<_Statement*>(subprogram->compound));
}

string analyseFunctionCall(_FunctionCall* functionCall) {
	if (functionCall == NULL) {
		cout << "[analyseFunctionCall] pointer of _FunctionCall is null" << endl;
		return "";
	}
	//www
	//_SymbolRecord *record = findSymbolRecord(currSymbolTable, functionCall->functionId.first);
	_SymbolRecord* record = findSymbolRecord(mainSymbolTable, functionCall->functionId.first);
	if (record == NULL)
		record = findSymbolRecord(currSymbolTable, functionCall->functionId.first, 1);
	if (record == NULL) {    
		addUndefinedError(functionCall->functionId.first, functionCall->functionId.second);
		return functionCall->returnType = "error";
	}
	if (record->flag != "function") {//不是函数   
		addPreFlagError(functionCall->functionId.first, functionCall->functionId.second, "function", record->lineNumber, record->flag);
		return functionCall->returnType = "error";
	}
	if (record->amount == -1) {//如果是变参函数（本编译器涉及的变参函数对参数类型没有要求，但不能为error）
		for (auto iter : functionCall->actualParaList)
			analyseExpression(iter);
		return functionCall->returnType = record->type;
	}
	if (functionCall->actualParaList.size() != record->amount) {//参数个数不一致   
		addNumberError(functionCall->functionId.first, functionCall->functionId.second, int(functionCall->actualParaList.size()), record->amount, "function");
		return functionCall->returnType = record->type;
	}
	//检查各位置的实参和形参类型是否一致 形参在符号表中的定位
	for (int i = 0; i < functionCall->actualParaList.size(); i++) {
		string actualType = analyseExpression(functionCall->actualParaList[i]);
		string formalType = record->getIdxParamType(i + 1);
		bool isRefered = record->isIdxParamRef(i + 1);
		if (isRefered && !(functionCall->actualParaList[i]->type == "var" && (functionCall->actualParaList[i]->variantReference->kind == "var" || functionCall->actualParaList[i]->variantReference->kind == "array"))) {
			//该表达式不能作为引用形参对应的实参   
			addGeneralError("[Referenced actual parameter error!] <Line " + itos(functionCall->actualParaList[i]->lineNumber) + "> The " + itos(i + 1) + "th actual parameter expression should be a normal variable、value parameter、referenced parameter or array element.");
			continue;
		}
		if (!isRefered) { //传值参数支持从integer到real的隐式类型转换
			if (actualType != formalType && !(actualType == "integer" && formalType == "real"))  
				addExpressionTypeError(functionCall->actualParaList[i], actualType, formalType, itos(i + 1) + "th actual parameter of function call of \"" + functionCall->functionId.first + "\"");
		}
		else { //引用参数需保持类型强一致
			if (actualType != formalType)  
				addExpressionTypeError(functionCall->actualParaList[i], actualType, formalType, itos(i + 1) + "th actual parameter of function call of \"" + functionCall->functionId.first + "\"");
		}
	}
	return functionCall->returnType = record->type;
}

string analyseExpression(_Expression* expression) {
	if (expression == NULL) {
		cout << "[analyseExpression] pointer of _Expression is null" << endl;
		return "";
	}
	if (expression->type == "var") { //获得普通变量/常量/数组的类型 //如果是integer类型的常量
		string variantReferenceType = analyseVariantRef(expression->variantReference);
		if (variantReferenceType == "integer" && expression->variantReference->kind == "constant") {//int类型的常量
			//查符号表查出常量值
			_SymbolRecord* record = findSymbolRecord(currSymbolTable, expression->variantReference->variantId.first);
			if (record == NULL) {
				cout << "[analyseExpression] pointer of record is null" << endl;
				return "";
			}
			if (record->flag != "constant") {
				cout << "[analyseExpression] the record should be a constant" << endl;
				return "";
			}
			expression->totalIntValue = str2int(record->value);
			if (record->isMinusShow)
				expression->totalIntValue = -expression->totalIntValue;
			expression->totalIntValueValid = true;
		}
		return expression->expressionType = variantReferenceType;
	}
	else if (expression->type == "integer") {
		expression->totalIntValue = expression->intNum;
		expression->totalIntValueValid = true;
		return expression->expressionType = "integer";
	}
	else if (expression->type == "real")
		return expression->expressionType = "real";
	else if (expression->type == "char")
		return expression->expressionType = "char";
	else if (expression->type == "function") //获得函数调用的返回值类型
		return expression->expressionType = analyseFunctionCall(expression->functionCall);
	else if (expression->type == "compound") {
		if (expression->operationType == "relop") {
			string epType1 = analyseExpression(expression->operand1);
			string epType2 = analyseExpression(expression->operand2);
			if ((epType1 == epType2 && epType1 != "error") || (epType1 == "integer" && epType2 == "real") || (epType1 == "real" && epType2 == "integer"))
				return expression->expressionType = "boolean";
			else {
				if (epType1 != epType2 && epType1 != "error" && epType2 != "error")  
					addOperandExpressionsTypeMismatchError(expression->operand1, expression->operand2);
				return expression->expressionType = "error";
			}
		}
		else if (expression->operation == "not") {
			string type = analyseExpression(expression->operand1);
			if (type == "boolean")
				return expression->expressionType = "boolean";
			else {
				if (type != "error" && type != "boolean")  
					addSingleOperandExpressionTypeMismatchError(expression->operand1, "boolean");
				return expression->expressionType = "error";
			}
		}
		else if (expression->operation == "minus") {
			string epType = analyseExpression(expression->operand1);
			if (epType == "integer" && expression->operand1->totalIntValueValid) {
				expression->totalIntValue = -expression->operand1->totalIntValue;
				expression->totalIntValueValid = true;
			}
			if (epType == "integer" || epType == "real")
				return expression->expressionType = epType;
			else {
				if (epType != "error" && epType != "integer" && epType != "real")  
					addSingleOperandExpressionTypeMismatchError(expression->operand1, "integer or real");
				return expression->expressionType = "error";
			}
		}
		else if (expression->operation == "bracket") {
			expression->expressionType = analyseExpression(expression->operand1);
			if (expression->expressionType == "integer" && expression->operand1->totalIntValueValid) {
				expression->totalIntValue = expression->operand1->totalIntValue;
				expression->totalIntValueValid = true;
			}
			return expression->expressionType;
		}
		else if (expression->operation == "+" || expression->operation == "-" || expression->operation == "*" || expression->operation == "/") {
			string epType1 = analyseExpression(expression->operand1);
			string epType2 = analyseExpression(expression->operand2);
			 
			if (expression->operation == "/" && epType2 == "integer" && expression->operand2->totalIntValueValid && expression->operand2->totalIntValue == 0)
				addDivideZeroError(expression->operation, expression->operand2);
			if (epType1 == "integer" && epType2 == "integer" && expression->operand1->totalIntValueValid && expression->operand2->totalIntValueValid) {
				expression->totalIntValueValid = true;
				if (expression->operation == "+")
					expression->totalIntValue = expression->operand1->totalIntValue + expression->operand2->totalIntValue;
				else if (expression->operation == "-")
					expression->totalIntValue = expression->operand1->totalIntValue - expression->operand2->totalIntValue;
				else if (expression->operation == "*")
					expression->totalIntValue = expression->operand1->totalIntValue * expression->operand2->totalIntValue;
				else
					expression->totalIntValue = expression->operand1->totalIntValue / expression->operand2->totalIntValue;
			}
			if ((epType1 == "integer" || epType1 == "real") && (epType2 == "integer" || epType2 == "real")) {
				if (epType1 == "integer" && epType2 == "integer")
					return expression->expressionType = "integer";
				return expression->expressionType = "real";
			}
			if (epType1 != "error" && epType1 != "integer" && epType1 != "real")  
				addSingleOperandExpressionTypeMismatchError(expression->operand1, "integer or real");
			if (epType2 != "error" && epType2 != "integer" && epType2 != "real")  
				addSingleOperandExpressionTypeMismatchError(expression->operand2, "integer or real");
			return expression->expressionType = "error";
		}
		else if (expression->operation == "div" || expression->operation == "mod") {
			string epType1 = analyseExpression(expression->operand1);
			string epType2 = analyseExpression(expression->operand2);
			 
			if (epType2 == "integer" && expression->operand2->totalIntValueValid && expression->operand2->totalIntValue == 0)
				addDivideZeroError(expression->operation, expression->operand2);
			if (epType1 == "integer" && epType2 == "integer") {
				if (expression->operand1->totalIntValueValid && expression->operand2->totalIntValueValid) {
					if (expression->operation == "div")
						expression->totalIntValue = expression->operand1->totalIntValue / expression->operand2->totalIntValue;
					else
						expression->totalIntValue = expression->operand1->totalIntValue % expression->operand2->totalIntValue;
					expression->totalIntValueValid = true;
				}
				return expression->expressionType = "integer";
			}
			if (epType1 != "error" && epType1 != "integer")  
				addSingleOperandExpressionTypeMismatchError(expression->operand1, "integer");
			if (epType2 != "error" && epType2 != "integer")  
				addSingleOperandExpressionTypeMismatchError(expression->operand2, "integer");
			return expression->expressionType = "error";
		}
		else if (expression->operation == "and" || expression->operation == "or") {
			string epType1 = analyseExpression(expression->operand1);
			string epType2 = analyseExpression(expression->operand2);
			if (epType1 == "boolean" && epType2 == "boolean")
				return expression->expressionType = "boolean";
			if (epType1 != "error" && epType1 != "boolean")  
				addSingleOperandExpressionTypeMismatchError(expression->operand1, "boolean");
			if (epType2 != "error" && epType2 != "boolean")  
				addSingleOperandExpressionTypeMismatchError(expression->operand2, "boolean");
			return expression->expressionType = "error";
		}
		else {
			cout << "[_Expression::analyseExpression] ERROR: operation not found" << endl;
			return "error";
		}
	}
	else {
		//cout << "[_Expression::analyseExpression] ERROR: expression type not found" << endl;
		return "error";
	}
}

//对程序进行语义分析
void analysePrgm(_Program* program) {
	if (program == NULL) {
		cout << "[analysePrgm] pointer of _Program is null" << endl;
		return;
	}
	//库函数名、主程序名、主程序参数，在检查是否重定义时，优先级按照前面列举的顺序，
	//即主程序名不能和库函数名，主程序参数不能和库函数名、主程序名同名
	//添加主程序名、行号、参数个数等信息
	vector<string> lib;
	lib.emplace_back("read");
	lib.emplace_back("write");
	lib.emplace_back("writeln");
	lib.emplace_back("exit");
	mainSymbolTable->addProcedure("read", -1, -1, NULL);
	mainSymbolTable->addProcedure("write", -1, -1, NULL);
	mainSymbolTable->addProcedure("writeln", -1, -1, NULL);
	mainSymbolTable->addProcedure("exit", -1, 0, NULL);
	
	if(find(lib.begin(), lib.end(), program->programId.first) != lib.end()) //检查主程序名是否和库函数同名
		addGeneralError("[Duplicate defined error!] <Line " + itos(program->programId.second) + "> Name of program \"" + program->programId.first + "\" has been defined as a lib program.");
	mainSymbolTable->addPrgmName(program->programId.first, program->programId.second, "procedure", int(program->paraList.size()), "");

	for (int i = 0; i < program->paraList.size(); i++) {
		if (program->paraList[i].first == program->programId.first) //检查主程序参数是否和主程序名同名
			addGeneralError("[Duplicate defined error!] <Line " + itos(program->programId.second) + "> parameter of program \"" + program->programId.first + "\" is the same as name of program.");
		else if (find(lib.begin(), lib.end(), program->paraList[i].first) != lib.end()) //检查主程序参数是否和库函数同名
			addGeneralError("[Dulicate defined error!] <Line " + itos(program->paraList[i].second) + "> parameter of program \"" + program->paraList[i].first + "\" has been defined as a lib program.");
		mainSymbolTable->addVoidParam(program->paraList[i].first, program->paraList[i].second);
	}
	
	analyseSubPrgm(program->subProgram);
}

string itos(int num) {
	stringstream ssin;
	ssin << num;
	return ssin.str();
}

void addDuplicateDefinitionError(string preId, int preLineNumber, string preFlag, string preType, int curLineNumber) {
	string error = "[Duplicate defined error!] <Line " + itos(curLineNumber) + "> ";
	if (preLineNumber != -1)
		error += "\"" + preId + "\"" + " has already been defined as a " + preFlag + " at line " + itos(preLineNumber) + ".";
	else
		error += "\"" + preId + "\"" + " has already been defined as a lib program.";
	semanticError.push_back(error);
	CHECK_ERROR_BOUND
}

void addUndefinedError(string id, int curLineNumber) {
	string error;
	error = "[Undefined identifier!] <Line " + itos(curLineNumber) + "> ";
	error += id + " has not been defined.";
	semanticError.push_back(error);
	CHECK_ERROR_BOUND
}

void addUsageTypeError(string curId, int curLineNumber, string curType, string usage, string correctType) {
	string error;
	error = "[Usage type error!] <Line " + itos(curLineNumber) + "> ";
	error += "\"" + curId + "\"" + " used for " + usage + " should be " + correctType + " but not " + curType + ".";
	semanticError.push_back(error);
	CHECK_ERROR_BOUND
}

void addNumberError(string curId, int curLineNumber, int curNumber, int correctNumber, string usage) {
	string error;
	if (usage == "array") {
		error += "[Array index number mismatch!] ";
		error += "<Line " + itos(curLineNumber) + "> ";
		error += "Array \"" + curId + "\"" + " should have " + itos(correctNumber) + " but not " + itos(curNumber) + " indices.";
	}
	else if (usage == "procedure") {
		error += "[Procedure parameter number mismatch!] ";
		error += "<Line " + itos(curLineNumber) + "> ";
		error += "Procedure \"" + curId + "\"" + " should have " + itos(correctNumber) + " but not " + itos(curNumber) + " parameters.";
	}
	else if (usage == "function") {
		error += "[Function parameter number mismatch!] ";
		error += "<Line " + itos(curLineNumber) + "> ";
		error += "Function \"" + curId + "\"" + " should have " + itos(correctNumber) + " but not " + itos(curNumber) + " parameters.";
	}
	else {
		cout << "[addNumberError] usage error" << endl;
		return;
	}
	semanticError.push_back(error);
	CHECK_ERROR_BOUND
}

void addPreFlagError(string curId, int curLineNumber, string curFlag, int preLineNumber, string preFlag) {
	string error;
	error += "[Symbol kinds mismatch!] ";
	error += "<Line " + itos(curLineNumber) + "> ";
	error += "\"" + curId + "\"" + " defined at line " + itos(preLineNumber) + " is a " + preFlag + " but not a " + curFlag + ".";
	semanticError.push_back(error);
	CHECK_ERROR_BOUND
}

void addExpressionTypeError(_Expression* exp, string curType, string correctType, string description) {
	string error;
	error += "[Expression type error!] ";
	error += "<Line " + itos(exp->lineNumber) + "> ";
	string expression;
	getExpression(exp, expression, 1);
	error += "Expression \"" + expression + "\" used for " + description + " should be " + correctType + " but not " + curType + ".";
	semanticError.push_back(error);
	CHECK_ERROR_BOUND
}

void addAssignTypeMismatchError(_VariantReference* leftVariantReference, _Expression* rightExpression) {
	string error;
	error += "[Assign statement type mismatch!] ";
	error += "<Left at line " + itos(leftVariantReference->variantId.second) + ", right at line " + itos(rightExpression->lineNumber) + "> ";
	string varRef, exp;
	getVariantRef(leftVariantReference, varRef, 1);
	getExpression(rightExpression, exp, 1);
	error += "Left \"" + varRef + "\" type is " + leftVariantReference->variantType + " while right \"" + exp + "\" type is " + rightExpression->expressionType + ".";
	semanticError.push_back(error);
	CHECK_ERROR_BOUND
}

void addArrayRangeOutOfBoundError(_Expression* expression, string arrayId, int X, pair<int, int> range) {
	string error;
	error += "[Array range out of bound!] ";
	error += "<Line " + itos(expression->lineNumber) + "> ";
	string exp;
	getExpression(expression, exp, 1);
	error += "The value of expression \"" + exp + "\"" + " is " + itos(expression->totalIntValue);
	error += ", but the range of array \"" + arrayId + "\" " + itos(X) + "th index is " + itos(range.first) + " to " + itos(range.second) + ".";
	semanticError.push_back(error);
	CHECK_ERROR_BOUND
}

void addArrayRangeUpSideDownError(string curId, int curLineNumber, int X, int lowBound, int highBound) {
	string error;
	error += "[Array range upsidedown error!] ";
	error += "<Line " + itos(curLineNumber) + "> ";
	error += itos(X) + "th range of array \"" + curId + "\" have larger low bound and smaller high bound, which is " + itos(lowBound) + " and " + itos(highBound) + ".";
	semanticError.push_back(error);
	CHECK_ERROR_BOUND
}

void addOperandExpressionsTypeMismatchError(_Expression* exp1, _Expression* exp2) {
	string error;
	error += "[Operands expression type mismatch!] ";
	error += "<Left at line " + itos(exp1->lineNumber) + ", right at line " + itos(exp2->lineNumber) + "> ";
	string expStr1, expStr2;
	getExpression(exp1, expStr1, 1);
	getExpression(exp2, expStr2, 1);
	error += "Left \"" + expStr1 + "\" type is " + exp1->expressionType + " while right " + "\"" + expStr2 + "\" type is " + exp2->expressionType + ".";
	semanticError.push_back(error);
	CHECK_ERROR_BOUND
}

void addSingleOperandExpressionTypeMismatchError(_Expression* exp, string correctType) {
	string error;
	error += "[Operand expression type error!] ";
	error += "<Line " + itos(exp->lineNumber) + "> ";
	string expStr;
	getExpression(exp, expStr, 1);
	error += "Expression \"" + expStr + "\" type should be " + correctType + " but not " + exp->expressionType + ".";
	semanticError.push_back(error);
	CHECK_ERROR_BOUND
}

void addactualParameterOfReadError(int curLineNumber, string procedureId, int X, _Expression* exp) {
	string error;
	error += "[Actual parameter of read procedure type error!] ";
	error += "<Line " + itos(curLineNumber) + "> ";
	string expression;
	getExpression(exp, expression, 1);
	error += "\"" + procedureId + "\" " + itos(X) + "th expression parameter \"" + expression + "\" is not a variant or an array element.";
	semanticError.push_back(error);
	CHECK_ERROR_BOUND
}

void addDivideZeroError(string operation, _Expression* exp) {
	string error;
	error += "[Divide zero error!] ";
	error += "<Line " + itos(exp->lineNumber) + "> ";
	string expression;
	getExpression(exp, expression, 1);
	error += "The value of expression \"" + expression + "\" is 0, which is the second operand of operation \"" + operation + "\".";
	semanticError.push_back(error);
	CHECK_ERROR_BOUND
}

void addReadBooleanError(_Expression* exp, int X) {
	string error;
	string expression;
	getExpression(exp, expression, 1);
	error = "[Read boolean error!] ";
	error += "<Line " + itos(exp->lineNumber) + "> ";
	error += "The " + itos(X) + "th actual parameter of read \"" + expression + "\" is boolean, it can't be read.";
	semanticError.push_back(error);
	CHECK_ERROR_BOUND
}

void addGeneralError(string error) {
	semanticError.push_back(error);
	CHECK_ERROR_BOUND
}

bool isIdIllgal(string id, int lineNumber) { //这里的Key指的是库程序名、主程序名、主程序参数
	for (int i = 0; i <= mainSymbolTable->recordList[0]->amount + 4; i++) {
		if (id == mainSymbolTable->recordList[i]->id) {
			if (i == 0) //与主程序名同名   
				addGeneralError("[Duplicate defined error!] <Line " + itos(lineNumber) + "> \"" + id + "\" has been defined as the name of program at Line " + itos(mainSymbolTable->recordList[i]->lineNumber) + ".");
			else if (i >= 1 && i <= mainSymbolTable->recordList[0]->amount) //与主程序参数同名   
				addGeneralError("[Duplicate defined error!] <Line " + itos(lineNumber) + "> \"" + id + "\" has been defined as a program parameter at Line " + itos(mainSymbolTable->recordList[i]->lineNumber) + ".");
			else //与库程序同名   
				addGeneralError("[Duplicate defined error!] <Line " + itos(lineNumber) + "> \"" + id + "\" has been defined as a lib program.");
			return true;
		}
	}
	return false;
}