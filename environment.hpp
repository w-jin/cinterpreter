#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <iostream>
#include <map>
#include <string>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;

class StackFrame {
private:
    /// StackFrame maps Variable Declaration to Value
    /// Which are either integer or addresses (also represented using an Integer value)
    std::map<Decl*, long> mVars;
    std::map<Stmt*, long> mExprs;   //值的类型设为long，保证保存地址时不溢出
    /// The current stmt
    Stmt *mPC;

    //表征函数是否已经返回
    bool _hasReturn;

  public:
    StackFrame() : mVars(), mExprs(), mPC(), _hasReturn(false) {
    }

    //更新和获取变量的值
    void bindDecl(Decl *decl, long val) {
        mVars[decl] = val;
    }
    long getDeclVal(Decl *decl) {
        assert (mVars.find(decl) != mVars.end());
        return mVars.find(decl)->second;
    }

    //更新和获取表达式的值
    void bindStmt(Stmt *stmt, long val) {
		  mExprs[stmt] = val;
    }
    long getStmtVal(Stmt *stmt) {
		  assert (mExprs.find(stmt) != mExprs.end());
		  return mExprs[stmt];
    }

    //这两个函数用于函数调用返回地址设置
    void setPC(Stmt *stmt) {
		mPC = stmt;
    }
    Stmt *getPC() {
		return mPC;
    }

    //设置和检查函数是否return
    void setReturn(bool flag) {
        _hasReturn = flag;
    }
    bool getReturn() {
        return _hasReturn;
    }

    //以下为对变量表的查找遍历接口
    std::map<Decl*, long>::iterator mVars_begin() {
        return mVars.begin();
    }
    std::map<Decl*, long>::iterator mVars_end() {
        return mVars.end();
    }
    std::map<Decl*, long>::iterator mVars_find(Decl *decl) {
        return mVars.find(decl);
    }
};

/* 内存管理，以long为单元进行管理，内存地址表示是第几个long，0地址表示nullptr
 * 所有数组和指针的地址都是从0开始的虚拟地址，所有求地址的表达式都将分配一个虚拟地址，
 * 并记录虚拟地址到实际地址的映射关系
 */
class Heap {
public:
    Heap() : mBuffers(), mValues(), mPointers(), min_addr(1) {}

    //分配内存，直接分配size个long类型单元
    long Malloc(long size) {
        long buffer = min_addr;
        min_addr = min_addr + size;
        mBuffers[buffer] = size;

        for (long i = 0; i < size; ++i)
            mValues[buffer + i] = 0;

        return buffer;
    }

    //释放内存，释放的内存必须是通过Malloc分配的
    void Free(long buffer) {
        if (0 == buffer)    //保证空指针不出错
            return ;
        assert(mBuffers.find(buffer) != mBuffers.end());
        long size = mBuffers[buffer];
        for (long i = 0; i < size; ++i)
            mValues.erase(mValues.find(buffer + i));
        mBuffers.erase(mBuffers.find(buffer));
        
        //如果释放的内存位于分配的内存末尾，可以减小min_addr的值，其它情况不再考虑
        if (buffer + size == min_addr)
            min_addr = buffer;
    }

    //更新某个地址的值
    void Update(long addr, long val) {
        // if (addr > min_addr || addr <= 0)
        //    std::cout << "Segment Default!" << std::endl;
        assert(mValues.find(addr) != mValues.end());
        mValues[addr] = val;
    }

    //获取某个地址的值
    long Get(long addr) {
        // if (addr > min_addr || addr <= 0)
        //    std::cout << "Segment Default!" << std::endl;
        assert(mValues.find(addr) != mValues.end());
        return mValues[addr];
    }

    //获取某个地址的实际地址
    Expr *getRealAddr(long addr) {
        if (mPointers.find(addr) != mPointers.end())
            return mPointers[addr];     //普通变量的指针将返回其表达式指针，随后更改其实际地址处的值
        else
            return nullptr;             //数组元素的指针不需要更改实际地址处的值
    }

    //获取某个实际地址的虚拟地址，如果没有，则返回0，表明需要分配
    long getImageAddr(Expr *addr) {
        for (auto i = mPointers.begin(), e = mPointers.end(); i != e; ++i)
            if (i->second == addr)
                return i->first;

        return 0;
    }

    //更新某个虚拟地址的实际地址
    void UpdatePointer(long addr, Expr *expr) {
        assert(mPointers.find(addr) != mPointers.end());
        mPointers[addr] = expr;
    }

  private:
    //保存分配的内存的首地址和大小
    std::map<long, long> mBuffers;
    //保存某内存地址的值，使用long保证指针不溢出，指针中保存的地址也放在这里面
    std::map<long, long> mValues;
    //指针地址映射表，将mValues中保存的指针地址映射为实际地址
    std::map<long, Expr *> mPointers;
    //尚未分配的最小地址
    long min_addr;
};

class Environment {
private:
    //保存当前栈上的变量
    std::vector<StackFrame> mStack;
    //保存全局变量，所有栈帧以它为模板进行创建，每次撤销栈帧也要将全局变量的值更新一次
    StackFrame mGlobalVars;
    //保存分配的内存
    Heap mHeap;

    /// Declartions to the built-in functions
    FunctionDecl *mFree;
    FunctionDecl *mMalloc;
    FunctionDecl *mInput;
    FunctionDecl *mOutput;
    FunctionDecl *mEntry;
public:
    /// Get the declartions to the built-in functions
    Environment() : mStack(), mFree(NULL), mMalloc(NULL), mInput(NULL), mOutput(NULL), mEntry(NULL) {
    }


    /// Initialize the Environment
    void init(TranslationUnitDecl *unit) {
		for (TranslationUnitDecl::decl_iterator i =unit->decls_begin(), e = unit->decls_end(); i != e; ++ i) {
            //识别内建函数和主函数
			if (FunctionDecl *fdecl = dyn_cast<FunctionDecl>(*i) ) {
				if (fdecl->getName().equals("free")) mFree = fdecl;
				else if (fdecl->getName().equals("malloc")) mMalloc = fdecl;
				else if (fdecl->getName().equals("get")) mInput = fdecl;
				else if (fdecl->getName().equals("print")) mOutput = fdecl;
				else if (fdecl->getName().equals("main")) mEntry = fdecl;
			}
            //处理全局变量，必须以字面值常量初始化，不能以表达式或变量进行初始化
            if (VarDecl *vdecl = dyn_cast<VarDecl>(*i)) {
                int val = 0;
                if (!(vdecl->hasInit())) {  //未初始化的初始化为0
                    if (!(vdecl->getType()->isArrayType()))   //不是数组类型的绑定为0
                        mGlobalVars.bindDecl(vdecl, 0);
                    else {
                        //将类型转为字符串(形如int [10])，从里面分析出下标大小
                        //TODO:有没有更优雅的方法呢？
                        std::string type = (vdecl->getType()).getAsString();
                        int size = 0;
                        int indexLeft = type.find("[");
						int indexRight = type.find("]");
						if(((size_t)indexLeft != std::string::npos) && ((size_t)indexRight != std::string::npos))
						{
                            std::string num = type.substr(indexLeft + 1, indexRight - indexLeft - 1);
							size = atoi(num.c_str());
						}
					
						//分配内存并保存首地址
						long buf = mHeap.Malloc(size);
                        mGlobalVars.bindDecl(vdecl, buf);
                    }
                }
                else {  //有初始值的，只处理整型变量
                    IntegerLiteral *integer = dyn_cast<IntegerLiteral>(vdecl->getInit());
                    val = integer->getValue().getSExtValue();
                    mGlobalVars.bindDecl(vdecl, val);
                }
            }
        }

		mStack.push_back(mGlobalVars);
    }

    //主函数入口
    FunctionDecl *getEntry() {
		return mEntry;
    }

    /// 二元操作：=、+、-、*、/、%、比较
    void binop(BinaryOperator *bop) {
		Expr *left = bop->getLHS();    //左操作数
		Expr *right = bop->getRHS();   //右操作数

        //赋值
		if (bop->isAssignmentOp()) {
			long val = mStack.back().getStmtVal(right);

            /* 处理左值表达式：指针和数组下标引用 */
            //指针
            if (isa<UnaryOperator>(left)) {
                UnaryOperator *uop = dyn_cast<UnaryOperator>(left);
                if (uop->getOpcode() == UO_Deref) { //确定是指针
                    Expr *sub_expr = uop->getSubExpr();
                    long addr = mStack.back().getStmtVal(sub_expr);
                    mHeap.Update(addr, val);    //更新虚地址中的值
                    //更新实际地址中的值
                    if (Expr *expr = mHeap.getRealAddr(addr)) {
                        DeclRefExpr *declexpr = dyn_cast<DeclRefExpr>(expr);
                        Decl *decl = declexpr->getFoundDecl();
                        mStack.back().bindDecl(decl, val);
                    }
                }
            }
            //数组
            else if (isa<ArraySubscriptExpr>(left)) {
                ArraySubscriptExpr *array = dyn_cast<ArraySubscriptExpr>(left);
                Expr *left_expr = array->getLHS();  //数组名
                Expr *right_expr = array->getRHS(); //下标

                long base = mStack.back().getStmtVal(left_expr);
                long offset = mStack.back().getStmtVal(right_expr);
                mHeap.Update(base + offset, val);
            }
            //其它变量赋值，左边必为变量名，直接更新至变量引用表
            else if (DeclRefExpr *declexpr = dyn_cast<DeclRefExpr>(left)) {
                Decl *decl = declexpr->getFoundDecl();
                mStack.back().bindDecl(decl, val);
            }

            //整个表达式的值为赋值号右边的值，有了这个可以支持连等
            mStack.back().bindStmt(bop, val);
		}

        //加减法
		//TODO:指针加减法，应该是可以了
        else if (bop->isAdditiveOp()) {
            long val1 = mStack.back().getStmtVal(left);
            long val2 = mStack.back().getStmtVal(right);
            if (bop->getOpcode() == BO_Add)         //+
                mStack.back().bindStmt(bop, val1 + val2);
            else if (bop->getOpcode() == BO_Sub)    //-
                mStack.back().bindStmt(bop, val1 - val2);
        }

        //乘除法
        else if (bop->isMultiplicativeOp()) {
            int val1 = mStack.back().getStmtVal(left);
            int val2 = mStack.back().getStmtVal(right);
            if (bop->getOpcode() == BO_Mul)         //*
                mStack.back().bindStmt(bop, val1 * val2);
            else if (bop->getOpcode() == BO_Div)    //除
                mStack.back().bindStmt(bop, val1 / val2);
            else if (bop->getOpcode() == BO_Rem)    //%
                mStack.back().bindStmt(bop, val1 % val2);
        }

        //比较操作符
        else if (bop->isComparisonOp()) {
            int val1 = mStack.back().getStmtVal(left);
            int val2 = mStack.back().getStmtVal(right);
            int result = 0;

            switch (bop->getOpcode()) {
            case BO_LT:     //<
                result = val1 < val2;
                break;
            case BO_GT:     //>
                result = val1 > val2;
                break;
            case BO_LE:     //<=
                result = val1 <= val2;
                break;
            case BO_GE:     //>=
                result = val1 >= val2;
                break;
            case BO_EQ:     //==
                result = val1 == val2;
                break;
            case BO_NE:     //!=
                result = val1 != val2;
                break;
            default:
                ;
            }

            mStack.back().bindStmt(bop, result);
        }

        //逻辑运算
        else if (bop->isLogicalOp()) {
            int val1 = mStack.back().getStmtVal(left);
            int val2 = mStack.back().getStmtVal(right);
            if (bop->getOpcode() == BO_LAnd)         //&&
                mStack.back().bindStmt(bop, val1 && val2);
            else if (bop->getOpcode() == BO_LOr)    //||
                mStack.back().bindStmt(bop, val1 || val2);
        }
    }

    //一元操作符，+、-、*
    void unaryop(UnaryOperator *uop) {
        Expr *sub_expr = uop->getSubExpr();
        long val = mStack.back().getStmtVal(sub_expr);  //操作数
        long addr = 0;
        switch (uop->getOpcode())
        {
        case UO_PostInc:        //后置++
            mStack.back().bindStmt(uop, val);
            if (DeclRefExpr *declexpr = dyn_cast<DeclRefExpr>(sub_expr)) {
				Decl *decl = declexpr->getFoundDecl();
				mStack.back().bindDecl(decl, val + 1);
			}
            break;
        case UO_PostDec:        //后置--
            mStack.back().bindStmt(uop, val);
            if (DeclRefExpr *declexpr = dyn_cast<DeclRefExpr>(sub_expr)) {
				Decl *decl = declexpr->getFoundDecl();
				mStack.back().bindDecl(decl, val - 1);
			}            
            break;
        case UO_PreInc:         //前置++
            mStack.back().bindStmt(uop, val + 1);
            if (DeclRefExpr *declexpr = dyn_cast<DeclRefExpr>(sub_expr)) {
				Decl *decl = declexpr->getFoundDecl();
				mStack.back().bindDecl(decl, val + 1);
			}
            break;
        case UO_PreDec:         //前置--
            mStack.back().bindStmt(uop, val - 1);
            if (DeclRefExpr *declexpr = dyn_cast<DeclRefExpr>(sub_expr)) {
				Decl *decl = declexpr->getFoundDecl();
				mStack.back().bindDecl(decl, val - 1);
			}
            break;
        case UO_Plus:           //+
            mStack.back().bindStmt(uop, val);
            break;
        case UO_Minus:          //-
            mStack.back().bindStmt(uop, -val);
            break;
        case UO_LNot:           //!
            mStack.back().bindStmt(uop, !val);
            break;
        case UO_Deref:          //访问指针所指的内存
            mStack.back().bindStmt(uop, mHeap.Get(val));
            break;
        case UO_AddrOf:         //取地址
            addr = mHeap.getImageAddr(sub_expr);
            if (0 == addr) {
                addr = mHeap.Malloc(1);     //TODO:撤销栈帧时内存泄露
                mHeap.Update(addr, val);
                mHeap.UpdatePointer(addr, sub_expr);
            }
            mStack.back().bindStmt(uop, addr);
            break;
        default:                //其它操作符暂不处理
            ;
        }
    }

    //处理括号括起来的表达式
    void paren(ParenExpr *paren_expr) {
        Expr *sub_expr = paren_expr->getSubExpr();
        long val = mStack.back().getStmtVal(sub_expr);
        mStack.back().bindStmt(paren_expr, val);
    }

    //实现sizeof操作符
    //TODO:不能支持变量的sizeof操作，报段错误
    void sizeOf(UnaryExprOrTypeTraitExpr *uett) {
        int size;

        if (uett->getKind() == UETT_SizeOf) {
            if (uett->getArgumentType()->isIntegerType())   //整数
                size = sizeof(int);
            else if (uett->getArgumentType()->isPointerType())  //指针
                size = sizeof(int *);
        }

        mStack.back().bindStmt(uett, size);
    }

    //处理数组，比如A[i]
    void array(ArraySubscriptExpr *array_expr) {
        Expr *left = array_expr->getLHS();  //A
        Expr *right = array_expr->getRHS(); //i
        long base = mStack.back().getStmtVal(left);
        long offset = mStack.back().getStmtVal(right);

        mStack.back().bindStmt(array_expr, mHeap.Get(base + offset));
    }

    //取出语法树中的整数将它作为表达式插入到stack中
    void integerLiteral(IntegerLiteral *integer) {
        int val = integer->getValue().getSExtValue();
        mStack.back().bindStmt(integer, val);
    }

    //处理声明语句
    void decl(DeclStmt *declstmt) {
        //一条语句可能声明多个变量，所以需要遍历
		for (DeclStmt::decl_iterator it = declstmt->decl_begin(), ie = declstmt->decl_end();
				it != ie; ++ it) {
			Decl *decl = *it;
			if (VarDecl *vardecl = dyn_cast<VarDecl>(decl)) {
                if (!(vardecl->hasInit())) {    //未初始化
                    if (!(vardecl->getType()->isArrayType()))   //不是数组类型
						mStack.back().bindDecl(vardecl, 0);
                    else {  
                        //将类型转为字符串(形如int [10])，从里面分析出下标大小
                        //TODO:有没有更优雅的方法呢
                        std::string type = (vardecl->getType()).getAsString();
						int size = 0;
						int indexLeft = type.find("[");
						int indexRight = type.find("]");
						if(((size_t)indexLeft != std::string::npos) && ((size_t)indexRight != std::string::npos))
						{
                            std::string num = type.substr(indexLeft + 1, indexRight - indexLeft - 1);
							size = atoi(num.c_str());
						}
					
						//分配内存并保存首地址，TODO:撤销栈帧时内存泄露
						long buf = mHeap.Malloc(size);
						mStack.back().bindDecl(vardecl, buf);
                    }
                }
				else if (vardecl->hasInit()) {	//有初始值
					long val = mStack.back().getStmtVal(vardecl->getInit());
					mStack.back().bindDecl(vardecl, val);
				}

			}
		}
    }

	//将引用的变量的值放到栈上
    void declref(DeclRefExpr *declref) {
		mStack.back().setPC(declref);
        mStack.back().bindStmt(declref, 0); //默认初始化为0

		//处理整型变量
		if (declref->getType()->isIntegerType()) {
			Decl *decl = declref->getFoundDecl();
			int val = mStack.back().getDeclVal(decl);
			mStack.back().bindStmt(declref, val);
		}
		//指针变量
		else if (declref->getType()->isPointerType())
		{
			Decl *decl = declref->getFoundDecl();
			long val = mStack.back().getDeclVal(decl);
			mStack.back().bindStmt(declref, val);
		}
		//数组类型
		else if (declref->getType()->isArrayType()) {
			Decl *decl = declref->getFoundDecl();
			long val = mStack.back().getDeclVal(decl);
			mStack.back().bindStmt(declref, val);
		}
    }

    void cast(CastExpr *castexpr) {
		mStack.back().setPC(castexpr);
		if (castexpr->getType()->isIntegerType()) {
			Expr *expr = castexpr->getSubExpr();
			int val = mStack.back().getStmtVal(expr);
			mStack.back().bindStmt(castexpr, val);
		}
        else {  //保留扩展性
            Expr *expr = castexpr->getSubExpr();
			int val = mStack.back().getStmtVal(expr);
			mStack.back().bindStmt(castexpr, val);
        }
    }

    //检查函数是否已经return，如果函数已经return，不能接着执行后面的语句
    bool hasReturn() {
        return mStack.back().getReturn();
    }

    //函数调用前设置环境
    void call(CallExpr *callexpr) {
		mStack.back().setPC(callexpr);
		FunctionDecl *callee = callexpr->getDirectCallee();
		if (callee == mInput) {
			llvm::errs() << "Please input an integer: ";
            int val = 0;
            std::cin >> val;
            mStack.back().bindStmt(callexpr, val);
        }
        else if (callee == mOutput) {
			Expr *decl = callexpr->getArg(0);
			int val = mStack.back().getStmtVal(decl);
			llvm::errs() << val << '\n';
        }
        else if (callee == mMalloc) {
			Expr *decl = callexpr->getArg(0);
			int val = mStack.back().getStmtVal(decl);
            long buffer = mHeap.Malloc(val / sizeof(int));  //按int分配，如果是指针数组，则后面一半无用
            mStack.back().bindStmt(callexpr, buffer);       //返回值
        }
        else if (callee == mFree) {
            Expr *decl = callexpr->getArg(0);
			long val = mStack.back().getStmtVal(decl);
            mHeap.Free(val);
        }
        else {
            //要继承全局变量
            StackFrame stack = mGlobalVars;
            //分析参数
            auto pit = callee->param_begin();
            for (auto ait = callexpr->arg_begin(), aie = callexpr->arg_end(); 
                    ait != aie; ++ait, ++pit) {
                int val = mStack.back().getStmtVal(*ait);
                stack.bindDecl(*pit, val);
            }
            //设置这个函数为未返回过的
            stack.setReturn(false);
            //把这一帧压入
            mStack.push_back(stack);
        }
    }

    //处理返回语句
    void ret(ReturnStmt *retstmt) {
        //取得返回值
        Expr *ret_expr = retstmt->getRetValue();
        int val = mStack.back().getStmtVal(ret_expr);

        //返回值保存到上一个栈帧，主函数的返回值不作处理
        if (mStack.size() < 2)
            return ;
        Stmt *stmt = mStack[mStack.size() - 2].getPC(); //调用语句
        mStack[mStack.size() - 2].bindStmt(stmt, val);

        //设置这一个函数为返回了的
        mStack.back().setReturn(true);
    }

    //函数调用之后重新设置环境
    void afterCall(CallExpr *callexpr) {
        FunctionDecl *callee = callexpr->getDirectCallee();
        if (callee != mInput && callee != mOutput && callee != mMalloc && callee != mFree) {
            //更新全局变量到全局变量表
            for (auto i = mStack.back().mVars_begin(), e = mStack.back().mVars_end(); i != e; ++i)
                if (mGlobalVars.mVars_find(i->first) != mGlobalVars.mVars_end())
                    mGlobalVars.bindDecl(i->first, i->second);

            //更新全局变量到上一个栈帧
            for (auto i = mGlobalVars.mVars_begin(), e = mGlobalVars.mVars_end(); i != e; ++i)
                mStack[mStack.size() - 2].bindDecl(i->first, i->second);

            mStack.pop_back();
        }
    }

    //计算条件表达式的值
    bool caculateCond(Expr *cond) {
        return mStack.back().getStmtVal(cond);
    }
};

#endif  // ~ENVIRONMENT_HPP
