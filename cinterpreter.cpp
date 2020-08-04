#include <iostream>
#include <fstream>
#include <string>
#include <iterator>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;

#include "environment.hpp"

class InterpreterVisitor : public EvaluatedExprVisitor<InterpreterVisitor> {
public:
    explicit InterpreterVisitor(const ASTContext &context, Environment *env)
    : EvaluatedExprVisitor(context), mEnv(env) {}
    virtual ~InterpreterVisitor() {}

    /* 以下函数对抽象语法树进行遍历，每个函数调用都有一个栈帧，栈帧内记录了一个
     * 函数是否已经返回，如果已经返回，则后续代码都不执行，所以每个函数开头都有
     * 一段if语句用于判断函数是否已经返回
     */

    //处理二元操作符
    virtual void VisitBinaryOperator(BinaryOperator *bop) {
        if (mEnv->hasReturn()) {
            return ;
        }

        VisitStmt(bop);
        mEnv->binop(bop);
    }

    //处理一元操作符
    virtual void VisitUnaryOperator(UnaryOperator *uop) {
        if (mEnv->hasReturn()) {
            return ;
        }

        VisitStmt(uop);
        mEnv->unaryop(uop);
    }

    //处理括号括起来的表达式
    virtual void VisitParenExpr(ParenExpr *paren_expr) {
        if (mEnv->hasReturn()) {
            return ;
        }

        VisitStmt(paren_expr);
        mEnv->paren(paren_expr);
    }

    //sizeof
    virtual void VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr *uett) {
        if (mEnv->hasReturn()) {
            return ;
        }

        VisitStmt(uett);
        mEnv->sizeOf(uett);
    }

    //处理数组下标访问
    virtual void VisitArraySubscriptExpr(ArraySubscriptExpr *array_expr) {
        if (mEnv->hasReturn()) {
            return ;
        }

        VisitStmt(array_expr);
        mEnv->array(array_expr);
    }
    
    //分析声明的变量
    virtual void VisitDeclStmt(DeclStmt *declstmt) {
        if (mEnv->hasReturn()) {
            return ;
        }

        //TODO:int a=1, b=2, c=a+b;不能被正确执行
        VisitStmt(declstmt);    //如果有初始值，应该先计算出初始值，否则调用decl时找不到值
	    mEnv->decl(declstmt);
    }
    
    //将引用的变量的值放到栈上
    virtual void VisitDeclRefExpr(DeclRefExpr *expr) {
        if (mEnv->hasReturn()) {
            return ;
        }

	    VisitStmt(expr);
	    mEnv->declref(expr);
    }

    //处理整数字面值，比如100、10
    virtual void VisitIntegerLiteral(IntegerLiteral *integer) {
        if (mEnv->hasReturn()) {
            return ;
        }

        mEnv->integerLiteral(integer);
    }

    virtual void VisitCastExpr(CastExpr *expr) {
        if (mEnv->hasReturn()) {
            return ;
        }

	    VisitStmt(expr);
	    mEnv->cast(expr);
    }

    //处理函数调用
    virtual void VisitCallExpr(CallExpr *call) {
        if (mEnv->hasReturn()) {
            return ;
        }

	    VisitStmt(call);
	    mEnv->call(call);   //设置好环境
        //执行函数体
        FunctionDecl *callee = call->getDirectCallee();
        Stmt *body = callee->getBody();
        if (body)
            VisitStmt(body);
        //重新设置环境，以执行函数调用后面的语句
        mEnv->afterCall(call);
    }

    //处理函数返回语句
    virtual void VisitReturnStmt(ReturnStmt *retstmt) {
        if (mEnv->hasReturn()) {
            return ;
        }

        VisitStmt(retstmt);
        mEnv->ret(retstmt);
    }

    //处理if语句(支持分支内定义新变量)，if/for/while语句块不新建栈帧，
    //因为这些块内声明的变量在块外引用时将引起语法分析器的错误状态
    virtual void VisitIfStmt(IfStmt *ifstmt) {
        if (mEnv->hasReturn()) {
            return ;
        }

        //计算条件表达式
        Expr *cond_expr = ifstmt->getCond();
        Visit(cond_expr);      //TODO:为什么不能使用VistStmt(cond_expr)
        bool cond = mEnv->caculateCond(cond_expr);

        //根据条件表达式选择分支
        if (cond) {
            Stmt *then_body = ifstmt->getThen();
            if (then_body)
                Visit(then_body);   //此处使用VisitStmt不能访问不加花括号的if语句
        }
        else {
            Stmt *else_body = ifstmt->getElse();
            if (else_body)
                Visit(else_body);
        }
    }

    //处理while语句(循环体内可以声明新变量)
    virtual void VisitWhileStmt(WhileStmt *whilestmt) {
        if (mEnv->hasReturn()) {
            return ;
        }

        //计算条件表达式
        Expr *cond_expr = whilestmt->getCond();
        Visit(cond_expr);
        bool cond = mEnv->caculateCond(cond_expr);

        //根据条件进行循环
        Stmt *while_body = whilestmt->getBody();
        while (cond) {
            if (while_body)
                Visit(while_body);      //TODO:可能无法访问不加花括号的while语句, 已解决
            //更新循环条件
            Visit(cond_expr);
            cond = mEnv->caculateCond(cond_expr);
        }
    }

    //处理for语句(循环体内可以声明新变量，不清楚是否为概率行为)
    virtual void VisitForStmt(ForStmt *forstmt) {
        if (mEnv->hasReturn()) {
            return ;
        }

        //初始化，TODO:初始化语句不能声明新变量
        Stmt *init_stmt = forstmt->getInit();
        if (init_stmt)
            VisitStmt(init_stmt);

        //循环条件
        Expr *cond_expr = forstmt->getCond();
        Visit(cond_expr);
        bool cond = mEnv->caculateCond(cond_expr);

        //循环体
        Stmt *for_body = forstmt->getBody();    //循环体
        Expr *inc_expr = forstmt->getInc();     //迭代表达式，如i++
        while (cond)
        {
            if (for_body)
                Visit(for_body);
            if (inc_expr)
                Visit(inc_expr);
            
            //更新循环条件
            Visit(cond_expr);
            cond = mEnv->caculateCond(cond_expr);
        }
    }

  private:
    Environment *mEnv;
};

class InterpreterConsumer : public ASTConsumer {
public:
    explicit InterpreterConsumer(const ASTContext& context) : mEnv(),
    	    mVisitor(context, &mEnv) {
    }
    virtual ~InterpreterConsumer() noexcept {}

    virtual void HandleTranslationUnit(clang::ASTContext &Context) {
	    TranslationUnitDecl *decl = Context.getTranslationUnitDecl();
	    mEnv.init(decl);

	    FunctionDecl *entry = mEnv.getEntry();
	    mVisitor.VisitStmt(entry->getBody());
    }
private:
    Environment mEnv;
    InterpreterVisitor mVisitor;
};

class InterpreterClassAction : public ASTFrontendAction {
public: 
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
            return std::unique_ptr<clang::ASTConsumer>(
                new InterpreterConsumer(Compiler.getASTContext()));
        }
};

int main (int argc, char **argv) {
    if (argc > 1) {
        std::ifstream source_file(argv[1]);
        std::string source(std::istreambuf_iterator<char>{source_file},
                           std::istreambuf_iterator<char>{});
        clang::tooling::runToolOnCode(new InterpreterClassAction, source);
    } else {
        std::cerr << "Please input .c file" << std::endl;
        return -1;
    }

    return 0;
}
