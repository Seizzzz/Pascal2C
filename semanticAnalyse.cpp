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
extern void getVariantRef(_VariantReference* variantRefNode, string& variantRef, int mode = 0, bool isReferedActualPara = false);//��ȡ��������
extern int str2int(string str);

vector<string> semanticError; //�洢������Ϣ������

void SemanticAnalyse(_Program* ASTRoot);

void newMainSymbolTable();//���������ű���ʼ��
void newSubSymbolTable();//�����ӷ��ű���ʼ��

string analyseVariantRef(_VariantReference* variantReference); 
void analyseStatement(_Statement* statement); 
void analyseSubPrgmDefine(_FunctionDefinition* functionDefinition); 
void analyseVariant(_Variant* variant); 
void analyseConst(_Constant* constant); 
void analyseSubPrgm(_SubProgram* subprogram); 
void analysePrgm(_Program* program); //������������������
void analyseFormalParam(_FormalParameter* formalParameter); 
string analyseFunctionCall(_FunctionCall* functionCall); 
string analyseExpression(_Expression* expression); 
bool isIdIllgal(string id, int lineNumber); //����ʶ���Ƿ������������������������������ͬ��

string itos(int num);//��intת��Ϊstring

void addDuplicateDefinitionError(string preId, int preLineNumber, string preFlag, string preType, int curLineNumber); //��ʶ���ض���
void addUndefinedError(string id, int curLineNumber); //��ʶ��δ����
void addUsageTypeError(string curId, int curLineNumber, string curType, string usage, string correctType); //��ʶ������(type)����
void addNumberError(string curId, int curLineNumber, int curNumber, int correctNumber, string usage); //����ά�������̻�������������ƥ��
void addPreFlagError(string curId, int curLineNumber, string curFlag, int preLineNumber, string preFlag); //��ʶ������(flag)����
void addExpressionTypeError(_Expression* exp, string curType, string correctType, string description); //���ʽ���ʹ���
void addAssignTypeMismatchError(_VariantReference* leftVariantReference, _Expression* rightExpression); //��ֵ�����ֵ����ֵ���Ͳ�ƥ��
void addArrayRangeOutOfBoundError(_Expression* expression, string arrayId, int X, pair<int, int> range); //�����±�Խ��
void addArrayRangeUpSideDownError(string curId, int curLineNumber, int X, int lowBound, int highBound); //�������½粻�Ϸ�
void addOperandExpressionsTypeMismatchError(_Expression* exp1, _Expression* exp2); //˫Ŀ������������Ͳ�ƥ��
void addSingleOperandExpressionTypeMismatchError(_Expression* exp, string correctType); //�������޺Ϸ��ĵ�Ŀ������
void addactualParameterOfReadError(int curLineNumber, string procedureId, int X, _Expression* exp); //readʵ�δ���
void addDivideZeroError(string operation, _Expression* exp); //�����Ҳ�����Ϊ0
void addReadBooleanError(_Expression* exp, int X); //���read��ȡboolean���ͱ����������Ϣ
void addGeneralError(string error); //����������Ϣ

void SemanticAnalyse(_Program* ASTRoot) {
	newMainSymbolTable();
	analysePrgm(ASTRoot);
}

void newMainSymbolTable() { //���������ű�
	currSymbolTable = mainSymbolTable = new _SymbolTable("main");//ָ��Ϊ�����ű�󣬻��Զ�����read, write�ȿ⺯��
}

void newSubSymbolTable() {//�����ӷ��ű� ���ű�λ
	currSymbolTable = new _SymbolTable("sub");//��������λ���ӷ��ű�
}

//�Ա������ý����������
//����������Ϊ��ֵ�������Ǵ�ֵ���������ò�������ͨ����������Ԫ�ء�����������������ֵ��
//����������Ϊ��ֵ�������Ǵ�ֵ���������ò�������ͨ����������Ԫ�ء������������������ĺ�����
string analyseVariantRef(_VariantReference* variantReference) {
	if (variantReference == NULL) {
		cout << "[analyseVariantRef] pointer of _VariantReference is null" << endl;
		return "";
	}
	_SymbolRecord* record = findSymbolRecord(currSymbolTable, variantReference->variantId.first);
	if (record == NULL) {   
		addUndefinedError(variantReference->variantId.first, variantReference->variantId.second);
		return variantReference->variantType = "error";//�޷��ҵ��������õ�����
	}
	if (variantReference->flag == 0) {//����Ƿ�����
		if (record->flag == "(sub)program name") {
			if (record->subprogramType == "procedure") {
				addGeneralError("[Invalid reference] <Line " + itos(variantReference->variantId.second) + "> Procedure name \"" + record->id + "\" can't be referenced");
				return variantReference->variantType = "error";
			}
			
			if (variantReference->locFlag == -1) { //-1��������ֵ
				variantReference->kind = "function return reference";
				return variantReference->variantType = record->type;
			}
			if (record->amount != 0) {//����βθ�����Ϊ0    �ݹ����
				addNumberError(variantReference->variantId.first, variantReference->variantId.second, 0, record->amount, "function");
				return variantReference->variantType = record->type;
			}
			//����βθ���Ϊ0
			//������Ӧ����������У�ʵ�������޲κ����ĵݹ����
			variantReference->kind = "function call";
			return variantReference->variantType = record->type;
		}
		if (record->flag == "function") {//����Ǻ��� ��һ�����������ű��в鵽��
			variantReference->kind = "function";
			//������Ϊ��ֵ��������Ϊ��ֵ�����βθ�������Ϊ0
			//��ʶ��ΪvariantReference�ĺ�������һ������ʵ�Σ�������Ҫ����βθ���
			if (variantReference->locFlag == -1) {//�������ֵ   
				addGeneralError("[Invalid reference!] <Line " + itos(variantReference->variantId.second) + "> function name \"" + record->id + "\" can't be referenced as l-value.");
				return variantReference->variantType = "error";
			}
			//�������ֵ
			if (record->amount != 0) {//����βθ�����Ϊ0   
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
	else if (variantReference->flag == 1) {//flag��1����������
		if (record->flag != "array") {//������ű��м�¼�Ĳ������� 
			addPreFlagError(variantReference->variantId.first, variantReference->variantId.second, "array", record->lineNumber, record->flag);
			return variantReference->variantType = "error";
		}
		variantReference->kind = "array";
		 
		if (variantReference->expressionList.size() != record->amount) {//�������ʱ���±�ά���ͷ��ű����治һ��
			addNumberError(variantReference->variantId.first, variantReference->variantId.second, int(variantReference->expressionList.size()), record->amount, "array");
			variantReference->variantType = "error";
			return record->type;
		}
		variantReference->variantType = record->type;
		for (int i = 0; i < variantReference->expressionList.size(); i++) {
			string type = analyseExpression(variantReference->expressionList[i]);
			//����±���ʽ�������Ƿ�������   
			if (type != "integer") {
				addExpressionTypeError(variantReference->expressionList[i], type, "integer", itos(i + 1) + "th index of array \"" + variantReference->variantId.first + "\"");
				variantReference->variantType = "error";
			}
			//���Խ��   
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
		if (type != "boolean") {//repeat������ͼ��,condition���ʽ���ͼ��   
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
		if (type != "boolean") {//while������ͼ��,condition���ʽ���ͼ��   
			addExpressionTypeError(whileStatement->condition, type, "boolean", "condition of while statement");
			whileStatement->statementType = "error";
		}
		else
			whileStatement->statementType = "void";
		analyseStatement(whileStatement->_do);
	}
	else if (statement->type == "for") {
		_ForStatement* forStatement = reinterpret_cast<_ForStatement*>(statement);
		//���ѭ�������Ƿ��Ѿ����壬���Ѿ����壬�Ƿ�Ϊinteger�ͱ���
		_SymbolRecord* record = findSymbolRecord(currSymbolTable, forStatement->id.first);
		if (record == NULL) {//ѭ������δ���壬������Ϣ   
			addUndefinedError(forStatement->id.first, forStatement->id.second);
			return;
		}
		//����޷���Ϊѭ������   
		if (!(record->flag == "value parameter" || record->flag == "var parameter" || record->flag == "normal variant")) {//�����ǰ�������಻������Ϊѭ������
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
		//��ѭ�����������������
		analyseStatement(forStatement->_do);
	}
	else if (statement->type == "if") {
		_IfStatement* ifStatement = reinterpret_cast<_IfStatement*>(statement);
		string type = analyseExpression(ifStatement->condition);
		if (type != "boolean") {//if������ͼ��,condition���ʽ���ͼ��   
			addExpressionTypeError(ifStatement->condition, type, "boolean", "condition of if statement");
			ifStatement->statementType = "error";
		}
		else
			ifStatement->statementType = "void";
		analyseStatement(ifStatement->then);//��then�������������
		if (ifStatement->els != NULL)//��else������������
			analyseStatement(ifStatement->els);
	}
	else if (statement->type == "assign") {//��ֵ����
		_AssignStatement* assignStatement = reinterpret_cast<_AssignStatement*>(statement);
		//����ֵ�������ý����������,���leftType
		assignStatement->statementType = "void";
		assignStatement->variantReference->locFlag = -1;//���Ϊ��ֵ
		string leftType = analyseVariantRef(assignStatement->variantReference);
		if (assignStatement->variantReference->kind == "constant") {
			//��ֵ����Ϊ����   
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
		//�Ƚ���ֵ����ֵ����,��ø�ֵ�������ͣ����Ͳ�ͬʱ��ֻ֧�����͵�ʵ�͵���ʽת��
		if (leftType != rightType && !(leftType == "real" && rightType == "integer")) {
			 
			addAssignTypeMismatchError(assignStatement->variantReference, assignStatement->expression);
			assignStatement->statementType = "error";
		}
		else
			assignStatement->statementType = "void";
	}
	else if (statement->type == "procedure") {//read�Ĳ���ֻ���Ǳ���������Ԫ��;
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
		if (record->flag != "procedure") {//������ǹ���   
			addPreFlagError(procedureCall->procedureId.first, procedureCall->procedureId.second, "procedure", record->lineNumber, record->flag);
			procedureCall->statementType = "error";
			return;
		}
		if (record->id == "exit") {
			/*exit�ȿ��Գ����ڹ����У�Ҳ���Գ����ں����У������ڹ�����ʱ��
			exit���ܴ������������ں�����ʱ��exitֻ�ܴ�һ��������
			�Ҹò������ʽ�����ͱ���ͺ����ķ���ֵ����һ��*/
			//�������жϵ�ǰ�����ǹ��̻��Ǻ���
			if (currSymbolTable->recordList[0]->subprogramType == "procedure") {//����ǹ���
				//exit���ܴ��������ʽ
				if (procedureCall->actualParaList.size() != 0) {//���ʵ�θ�����Ϊ0   
					addGeneralError("[Return value redundancy!] <Line " + itos(procedureCall->procedureId.second) + "> Number of return value of procedure must be 0, that is, exit must have no actual parameters.");
					procedureCall->statementType = "error";
				}
				return;
			}
			//����Ǻ���
			if (procedureCall->actualParaList.size() != 1) {//���ʵ�θ�����Ϊ1
				if (procedureCall->actualParaList.size() == 0)  
					addGeneralError("[Return value missing!] <Line " + itos(procedureCall->procedureId.second) + "> Number of return value of function must be 1, that is, exit must have 1 actual parameters.");
				else  
					addGeneralError("[Return value redundancy!] <Line " + itos(procedureCall->procedureId.second) + "> Number of return value of function must be 1, that is, exit must have 1 actual parameters.");
				return;
			}
			//���ʵ�θ���Ϊ1�����ʵ�α��ʽ�����ͣ�����Ƿ��뺯������ֵ����һ��
			string returnType = analyseExpression(procedureCall->actualParaList[0]);
			if (currSymbolTable->recordList[0]->type != returnType && !(currSymbolTable->recordList[0]->type == "real" && returnType == "integer")) {
				 
				addGeneralError("[Return type of funciton mismatch!] <Line " + itos(procedureCall->actualParaList[0]->lineNumber) + "> The type of return expression is " + returnType + " ,but not " + currSymbolTable->recordList[0]->type + " as function \"" + currSymbolTable->recordList[0]->id + "\" defined.");
				procedureCall->statementType = "error";
			}
			procedureCall->isReturnStatement = true;
			return;
		}
		if (record->id == "read" || record->id == "write") {
			if (procedureCall->actualParaList.size() == 0) { //read��write�Ĳ�����������Ϊ0   
				string tmp = record->id;
				tmp[0] -= 'a' - 'A';
				addGeneralError("[" + tmp + " actual parameter missing!] <Line " + itos(procedureCall->procedureId.second) + "> procedure \"" + record->id + "\" must have at least one actual parameter.");
				procedureCall->statementType = "error";
			}
		}
		if (record->id == "read") {//����ֻ���Ǳ���������Ԫ�أ������ǳ��������ʽ��
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
		if (record->amount == -1) {//����Ǳ�ι��̣����������漰�ı�ι���(����read)�Բ�������û��Ҫ�󣬵�����Ϊerror��
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
		// �β��ڷ��ű��еĶ�λ
		for (int i = 0; i < procedureCall->actualParaList.size(); i++) {//���actualParaList�����ʽ�����ͣ����ʵ�κ��βε�����һ����
			string actualType = analyseExpression(procedureCall->actualParaList[i]);
			string formalType = record->getIdxParamType(i + 1);
			bool isRefered = record->isIdxParamRef(i + 1);
			if (isRefered && !(procedureCall->actualParaList[i]->type == "var" && (procedureCall->actualParaList[i]->variantReference->kind == "var" || procedureCall->actualParaList[i]->variantReference->kind == "array"))) {
				//�ñ��ʽ������Ϊ�����βζ�Ӧ��ʵ��   
				addGeneralError("[Referenced actual parameter error!] <Line " + itos(procedureCall->actualParaList[i]->lineNumber) + "> The " + itos(i + 1) + "th actual parameter expression should be a normal variable��value parameter��referenced parameter or array element.");
				continue;
			}
			//if(isRefered && procedureCall->actualParaList[i]->type==)
			if (!isRefered) { //��ֵ����֧��integer��real����ʽ����ת��
				if (actualType != formalType && !(actualType == "integer" && formalType == "real")) { //������Ͳ�һ��
					 
					addExpressionTypeError(procedureCall->actualParaList[i], actualType, formalType, itos(i + 1) + "th actual parameter of procedure call of \"" + procedureCall->procedureId.first + "\"");
					procedureCall->statementType = "error";
				}
			}
			else { //���ò����豣������ǿһ��
				if (actualType != formalType) { //������Ͳ�һ��
					 
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

//����ʽ�������������������ʽ����һ���ǻ�������
void analyseFormalParam(_FormalParameter* formalParameter) {
	if (formalParameter == NULL) {
		cout << "[analyseFormalParam] pointer of _FormalParameter is null" << endl;
		return;
	}
	//return;
	_SymbolRecord* record = findSymbolRecord(currSymbolTable, formalParameter->paraId.first, 1);
	if (!isIdIllgal(formalParameter->paraId.first, formalParameter->paraId.second)) {
		if (record != NULL) //����ض���
			addDuplicateDefinitionError(record->id, record->lineNumber, record->flag, record->type, formalParameter->paraId.second);
	}
	//����Ƿ��뵱ǰ�������Լ�����֮ǰ������β�ͬ�������ͬ��������»��߽��лָ�����add�����н��У�
	if (formalParameter->flag == 0)
		currSymbolTable->addParam(formalParameter->paraId.first, formalParameter->paraId.second, formalParameter->type);
	else
		currSymbolTable->addVariantParam(formalParameter->paraId.first, formalParameter->paraId.second, formalParameter->type);
}

//���ӳ���������������
void analyseSubPrgmDefine(_FunctionDefinition* functionDefinition) {
	if (functionDefinition == NULL) {
		cout << "[analyseSubPrgmDefine] pointer of _FunctionDefinition is null" << endl;
		return;
	}
	_SymbolRecord* record = findSymbolRecord(currSymbolTable, functionDefinition->functionID.first, 1);
	if (record != NULL) {//�ض���   
		addDuplicateDefinitionError(record->id, record->lineNumber, record->flag, record->type, functionDefinition->functionID.second);
		return;
	}
	string subprogramType;
	if (functionDefinition->type.first.empty())
		subprogramType = "procedure";
	else
		subprogramType = "function";
	newSubSymbolTable(); //��������λ���ӷ��ű�
	//���ӳ���������Ϣ��ӵ��ӷ��ű���
	currSymbolTable->addPrgmName(functionDefinition->functionID.first, functionDefinition->functionID.second, subprogramType, int(functionDefinition->formalParaList.size()), functionDefinition->type.first);

	//����type�Ƿ�ΪNULL����ΪaddProcedure()��addFunction()����ӵ����������
	if (functionDefinition->type.first.empty())//����ǹ���
		mainSymbolTable->addProcedure(functionDefinition->functionID.first, functionDefinition->functionID.second, int(functionDefinition->formalParaList.size()), currSymbolTable);
	else//����Ǻ���
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
	_SymbolRecord* record = findSymbolRecord(currSymbolTable, variant->variantId.first, 1);//��variantId.firstȥ����ű�����Ƿ��ض���
	if (isIdIllgal(variant->variantId.first, variant->variantId.second))
		return;
	if (record != NULL) { //����ض���
		addDuplicateDefinitionError(record->id, record->lineNumber, record->flag, record->type, variant->variantId.second);
		return;
	}
	if (variant->type->flag == 0)//�������ͨ����
		currSymbolTable->addVariant(variant->variantId.first, variant->variantId.second, variant->type->type.first);
	else {//���������
		//���鶨��ʱ�����½���������Ͻ������ڵ����½磻�����ķ����壬���½��Ϊ�޷���������ͨ�����﷨��������һ�����޷�����
		vector<pair<int, int>>& tmp = variant->type->arrayRangeList;
		for (int i = 0; i < tmp.size(); i++) {
			if (tmp[i].first > tmp[i].second) {  
				addArrayRangeUpSideDownError(variant->variantId.first, variant->variantId.second, i + 1, tmp[i].first, tmp[i].second);
				tmp[i].second = tmp[i].first; //����Ͻ�С���½磬���Ͻ�����Ϊ�½�
			}
		}
		currSymbolTable->addArray(variant->variantId.first, variant->variantId.second, variant->type->type.first, int(variant->type->arrayRangeList.size()), variant->type->arrayRangeList);
	}
}

//�Գ�����������������
void analyseConst(_Constant* constant) {
	if (constant == NULL) {
		cout << "[analyseConst] pointer of _Constant is null" << endl;
		return;
	}
	//��constId.firstȥ����ű�����Ƿ��ض��� 
	_SymbolRecord* record = findSymbolRecord(currSymbolTable, constant->constId.first, 1);
	if (isIdIllgal(constant->constId.first, constant->constId.second))
		return;
	if (record != NULL) {//�ض���   
		addDuplicateDefinitionError(record->id, record->lineNumber, record->flag, record->type, constant->constId.second);
		return;
	}
	if (constant->type == "id") { //����ó���������ĳ�����ʶ������
		_SymbolRecord* preRecord = findSymbolRecord(currSymbolTable, constant->valueId.first);
		if (preRecord == NULL) {    
			addUndefinedError(constant->valueId.first, constant->valueId.second);
			return;
		}
		if (preRecord->flag != "constant") {//������ǳ���   
			addPreFlagError(constant->valueId.first, constant->valueId.second, "constant", preRecord->lineNumber, preRecord->flag);
			return;
		}
		currSymbolTable->addConst(constant->constId.first, constant->constId.second, preRecord->type, constant->isMinusShow ^ preRecord->isMinusShow, preRecord->value);
	}
	else//�ó����ɳ���ֵ����
		currSymbolTable->addConst(constant->constId.first, constant->constId.second, constant->type, constant->isMinusShow, constant->strOfVal);
}

//���ӳ�������������
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
		currSymbolTable = mainSymbolTable;//���ű��ض�λ
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
	if (record->flag != "function") {//���Ǻ���   
		addPreFlagError(functionCall->functionId.first, functionCall->functionId.second, "function", record->lineNumber, record->flag);
		return functionCall->returnType = "error";
	}
	if (record->amount == -1) {//����Ǳ�κ��������������漰�ı�κ����Բ�������û��Ҫ�󣬵�����Ϊerror��
		for (auto iter : functionCall->actualParaList)
			analyseExpression(iter);
		return functionCall->returnType = record->type;
	}
	if (functionCall->actualParaList.size() != record->amount) {//����������һ��   
		addNumberError(functionCall->functionId.first, functionCall->functionId.second, int(functionCall->actualParaList.size()), record->amount, "function");
		return functionCall->returnType = record->type;
	}
	//����λ�õ�ʵ�κ��β������Ƿ�һ�� �β��ڷ��ű��еĶ�λ
	for (int i = 0; i < functionCall->actualParaList.size(); i++) {
		string actualType = analyseExpression(functionCall->actualParaList[i]);
		string formalType = record->getIdxParamType(i + 1);
		bool isRefered = record->isIdxParamRef(i + 1);
		if (isRefered && !(functionCall->actualParaList[i]->type == "var" && (functionCall->actualParaList[i]->variantReference->kind == "var" || functionCall->actualParaList[i]->variantReference->kind == "array"))) {
			//�ñ��ʽ������Ϊ�����βζ�Ӧ��ʵ��   
			addGeneralError("[Referenced actual parameter error!] <Line " + itos(functionCall->actualParaList[i]->lineNumber) + "> The " + itos(i + 1) + "th actual parameter expression should be a normal variable��value parameter��referenced parameter or array element.");
			continue;
		}
		if (!isRefered) { //��ֵ����֧�ִ�integer��real����ʽ����ת��
			if (actualType != formalType && !(actualType == "integer" && formalType == "real"))  
				addExpressionTypeError(functionCall->actualParaList[i], actualType, formalType, itos(i + 1) + "th actual parameter of function call of \"" + functionCall->functionId.first + "\"");
		}
		else { //���ò����豣������ǿһ��
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
	if (expression->type == "var") { //�����ͨ����/����/��������� //�����integer���͵ĳ���
		string variantReferenceType = analyseVariantRef(expression->variantReference);
		if (variantReferenceType == "integer" && expression->variantReference->kind == "constant") {//int���͵ĳ���
			//����ű�������ֵ
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
	else if (expression->type == "function") //��ú������õķ���ֵ����
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

//�Գ�������������
void analysePrgm(_Program* program) {
	if (program == NULL) {
		cout << "[analysePrgm] pointer of _Program is null" << endl;
		return;
	}
	//�⺯����������������������������ڼ���Ƿ��ض���ʱ�����ȼ�����ǰ���оٵ�˳��
	//�������������ܺͿ⺯������������������ܺͿ⺯��������������ͬ��
	//��������������кš�������������Ϣ
	vector<string> lib;
	lib.emplace_back("read");
	lib.emplace_back("write");
	lib.emplace_back("writeln");
	lib.emplace_back("exit");
	mainSymbolTable->addProcedure("read", -1, -1, NULL);
	mainSymbolTable->addProcedure("write", -1, -1, NULL);
	mainSymbolTable->addProcedure("writeln", -1, -1, NULL);
	mainSymbolTable->addProcedure("exit", -1, 0, NULL);
	
	if(find(lib.begin(), lib.end(), program->programId.first) != lib.end()) //������������Ƿ�Ϳ⺯��ͬ��
		addGeneralError("[Duplicate defined error!] <Line " + itos(program->programId.second) + "> Name of program \"" + program->programId.first + "\" has been defined as a lib program.");
	mainSymbolTable->addPrgmName(program->programId.first, program->programId.second, "procedure", int(program->paraList.size()), "");

	for (int i = 0; i < program->paraList.size(); i++) {
		if (program->paraList[i].first == program->programId.first) //�������������Ƿ����������ͬ��
			addGeneralError("[Duplicate defined error!] <Line " + itos(program->programId.second) + "> parameter of program \"" + program->programId.first + "\" is the same as name of program.");
		else if (find(lib.begin(), lib.end(), program->paraList[i].first) != lib.end()) //�������������Ƿ�Ϳ⺯��ͬ��
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

bool isIdIllgal(string id, int lineNumber) { //�����Keyָ���ǿ���������������������������
	for (int i = 0; i <= mainSymbolTable->recordList[0]->amount + 4; i++) {
		if (id == mainSymbolTable->recordList[i]->id) {
			if (i == 0) //����������ͬ��   
				addGeneralError("[Duplicate defined error!] <Line " + itos(lineNumber) + "> \"" + id + "\" has been defined as the name of program at Line " + itos(mainSymbolTable->recordList[i]->lineNumber) + ".");
			else if (i >= 1 && i <= mainSymbolTable->recordList[0]->amount) //�����������ͬ��   
				addGeneralError("[Duplicate defined error!] <Line " + itos(lineNumber) + "> \"" + id + "\" has been defined as a program parameter at Line " + itos(mainSymbolTable->recordList[i]->lineNumber) + ".");
			else //������ͬ��   
				addGeneralError("[Duplicate defined error!] <Line " + itos(lineNumber) + "> \"" + id + "\" has been defined as a lib program.");
			return true;
		}
	}
	return false;
}